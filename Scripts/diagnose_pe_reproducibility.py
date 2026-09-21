#!/usr/bin/env python3
"""Compare two Windows PE images and localize byte reproducibility differences.

This diagnostic is intentionally read-only. It does not enable deterministic-linker
flags or claim that two files came from identical documented inputs. It reports
whether the supplied bytes match and, when they do not, whether every changed byte
falls inside recognized PE/COFF metadata fields such as timestamps or CodeView data.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import struct
import sys
import uuid
from dataclasses import dataclass
from pathlib import Path

MAX_IMAGE_BYTES = 512 * 1024 * 1024
MAX_SECTIONS = 96
MAX_DEBUG_ENTRIES = 4096
MAX_DIFF_RANGES = 256
MAX_PDB_PATH_BYTES = 4096


class PEFormatError(ValueError):
    pass


@dataclass(frozen=True)
class Section:
    name: str
    virtual_address: int
    virtual_size: int
    raw_offset: int
    raw_size: int


@dataclass(frozen=True)
class FieldRange:
    start: int
    end: int
    label: str


@dataclass(frozen=True)
class PEAnalysis:
    sha256: str
    size: int
    machine: int
    pe_format: str
    coff_timestamp: int
    checksum: int
    reproducible_debug_entry_present: bool
    debug_entries: tuple[dict[str, object], ...]
    sections: tuple[dict[str, object], ...]
    metadata_ranges: tuple[FieldRange, ...]


def _require_range(data: bytes, offset: int, size: int, label: str) -> None:
    if offset < 0 or size < 0 or offset > len(data) or size > len(data) - offset:
        raise PEFormatError(f"{label} extends outside file bounds")


def _u16(data: bytes, offset: int, label: str) -> int:
    _require_range(data, offset, 2, label)
    return struct.unpack_from("<H", data, offset)[0]


def _u32(data: bytes, offset: int, label: str) -> int:
    _require_range(data, offset, 4, label)
    return struct.unpack_from("<I", data, offset)[0]


def _decode_section_name(raw: bytes) -> str:
    return raw.split(b"\0", 1)[0].decode("ascii", errors="replace")


def _rva_to_offset(rva: int, sections: tuple[Section, ...], data_size: int, label: str) -> int:
    if rva == 0:
        raise PEFormatError(f"{label} has a null RVA")
    for section in sections:
        span = max(section.virtual_size, section.raw_size)
        if section.virtual_address <= rva < section.virtual_address + span:
            delta = rva - section.virtual_address
            if delta >= section.raw_size:
                raise PEFormatError(f"{label} points into virtual-only section data")
            offset = section.raw_offset + delta
            if not 0 <= offset < data_size:
                raise PEFormatError(f"{label} resolves outside file bounds")
            return offset
    raise PEFormatError(f"{label} RVA 0x{rva:x} is not mapped by a section")


def _bounded_c_string(data: bytes, offset: int, size: int) -> str:
    _require_range(data, offset, size, "CodeView data")
    raw = data[offset : offset + min(size, MAX_PDB_PATH_BYTES)]
    nul = raw.find(b"\0")
    if nul >= 0:
        raw = raw[:nul]
    return raw.decode("utf-8", errors="replace")


def analyze_pe(data: bytes) -> PEAnalysis:
    if len(data) < 64:
        raise PEFormatError("file is too small for a DOS header")
    if len(data) > MAX_IMAGE_BYTES:
        raise PEFormatError(f"file exceeds {MAX_IMAGE_BYTES} byte inspection limit")
    if data[:2] != b"MZ":
        raise PEFormatError("missing MZ header")
    pe_offset = _u32(data, 0x3C, "DOS e_lfanew")
    _require_range(data, pe_offset, 24, "PE and COFF headers")
    if data[pe_offset : pe_offset + 4] != b"PE\0\0":
        raise PEFormatError("missing PE signature")

    coff = pe_offset + 4
    machine = _u16(data, coff, "COFF machine")
    section_count = _u16(data, coff + 2, "COFF section count")
    coff_timestamp = _u32(data, coff + 4, "COFF timestamp")
    optional_size = _u16(data, coff + 16, "COFF optional-header size")
    if section_count == 0 or section_count > MAX_SECTIONS:
        raise PEFormatError(f"unreasonable section count: {section_count}")

    optional = coff + 20
    _require_range(data, optional, optional_size, "optional header")
    magic = _u16(data, optional, "optional-header magic")
    if magic == 0x10B:
        pe_format = "PE32"
        number_rvas_offset = 92
        data_directory_offset = 96
    elif magic == 0x20B:
        pe_format = "PE32+"
        number_rvas_offset = 108
        data_directory_offset = 112
    else:
        raise PEFormatError(f"unsupported optional-header magic 0x{magic:04x}")
    if optional_size < data_directory_offset:
        raise PEFormatError("optional header does not contain a data-directory table")

    checksum_offset = optional + 64
    checksum = _u32(data, checksum_offset, "optional-header checksum")
    number_rvas = _u32(data, optional + number_rvas_offset, "number of data directories")

    section_table = optional + optional_size
    _require_range(data, section_table, section_count * 40, "section table")
    sections: list[Section] = []
    section_reports: list[dict[str, object]] = []
    for index in range(section_count):
        offset = section_table + index * 40
        name = _decode_section_name(data[offset : offset + 8])
        virtual_size = _u32(data, offset + 8, "section virtual size")
        virtual_address = _u32(data, offset + 12, "section virtual address")
        raw_size = _u32(data, offset + 16, "section raw size")
        raw_offset = _u32(data, offset + 20, "section raw offset")
        if raw_size:
            _require_range(data, raw_offset, raw_size, f"section {name or index} raw data")
            raw_hash = hashlib.sha256(data[raw_offset : raw_offset + raw_size]).hexdigest()
        else:
            raw_hash = hashlib.sha256(b"").hexdigest()
        sections.append(Section(name, virtual_address, virtual_size, raw_offset, raw_size))
        section_reports.append(
            {
                "name": name,
                "virtual_address": virtual_address,
                "virtual_size": virtual_size,
                "raw_offset": raw_offset,
                "raw_size": raw_size,
                "raw_sha256": raw_hash,
            }
        )
    section_tuple = tuple(sections)

    metadata_ranges: list[FieldRange] = [
        FieldRange(coff + 4, coff + 8, "coff_timestamp"),
        FieldRange(checksum_offset, checksum_offset + 4, "optional_header_checksum"),
    ]
    debug_entries: list[dict[str, object]] = []
    reproducible = False

    if number_rvas > 6:
        rel = data_directory_offset + 6 * 8
        if rel + 8 > optional_size:
            raise PEFormatError("declared debug data directory exceeds optional header")
        debug_rva = _u32(data, optional + rel, "debug directory RVA")
        debug_size = _u32(data, optional + rel + 4, "debug directory size")
    else:
        debug_rva = 0
        debug_size = 0

    if debug_rva or debug_size:
        if not debug_rva or not debug_size:
            raise PEFormatError("debug directory has incomplete RVA/size")
        if debug_size % 28 != 0:
            raise PEFormatError("debug directory size is not a multiple of 28 bytes")
        count = debug_size // 28
        if count > MAX_DEBUG_ENTRIES:
            raise PEFormatError(f"too many debug directory entries: {count}")
        debug_offset = _rva_to_offset(debug_rva, section_tuple, len(data), "debug directory")
        _require_range(data, debug_offset, debug_size, "debug directory")
        for index in range(count):
            entry_offset = debug_offset + index * 28
            _, timestamp, major, minor, typ, size_of_data, address, pointer = struct.unpack_from(
                "<IIHHIIII", data, entry_offset
            )
            metadata_ranges.append(FieldRange(entry_offset + 4, entry_offset + 8, "debug_directory_timestamp"))
            entry: dict[str, object] = {
                "index": index,
                "type": typ,
                "timestamp": timestamp,
                "major_version": major,
                "minor_version": minor,
                "size_of_data": size_of_data,
                "address_of_raw_data": address,
                "pointer_to_raw_data": pointer,
            }
            if typ == 16:
                reproducible = True
                entry["kind"] = "reproducible"
            elif typ == 2:
                entry["kind"] = "codeview"
                if size_of_data:
                    _require_range(data, pointer, size_of_data, "CodeView raw data")
                    metadata_ranges.append(FieldRange(pointer, pointer + size_of_data, "codeview_data"))
                    if size_of_data >= 24 and data[pointer : pointer + 4] == b"RSDS":
                        guid_bytes = data[pointer + 4 : pointer + 20]
                        age = _u32(data, pointer + 20, "CodeView age")
                        entry["signature"] = "RSDS"
                        entry["guid"] = str(uuid.UUID(bytes_le=guid_bytes))
                        entry["age"] = age
                        entry["pdb_path"] = _bounded_c_string(data, pointer + 24, size_of_data - 24)
                    else:
                        entry["signature"] = data[pointer : pointer + min(4, size_of_data)].hex()
            else:
                entry["kind"] = "other"
            debug_entries.append(entry)

    return PEAnalysis(
        sha256=hashlib.sha256(data).hexdigest(),
        size=len(data),
        machine=machine,
        pe_format=pe_format,
        coff_timestamp=coff_timestamp,
        checksum=checksum,
        reproducible_debug_entry_present=reproducible,
        debug_entries=tuple(debug_entries),
        sections=tuple(section_reports),
        metadata_ranges=tuple(metadata_ranges),
    )


def _labels_for_range(start: int, end: int, metadata: tuple[FieldRange, ...]) -> list[str]:
    return sorted({field.label for field in metadata if start < field.end and end > field.start})


def _diff_ranges(a: bytes, b: bytes) -> tuple[list[dict[str, object]], int, bool]:
    limit = min(len(a), len(b))
    ranges: list[dict[str, object]] = []
    diff_count = 0
    start: int | None = None
    for index in range(limit):
        changed = a[index] != b[index]
        if changed:
            diff_count += 1
            if start is None:
                start = index
        elif start is not None:
            if len(ranges) < MAX_DIFF_RANGES:
                ranges.append({"start": start, "end": index, "length": index - start})
            start = None
    if start is not None and len(ranges) < MAX_DIFF_RANGES:
        ranges.append({"start": start, "end": limit, "length": limit - start})
    if len(a) != len(b):
        diff_count += abs(len(a) - len(b))
        if len(ranges) < MAX_DIFF_RANGES:
            ranges.append({"start": limit, "end": max(len(a), len(b)), "length": abs(len(a) - len(b))})

    logical_ranges = 0
    in_diff = False
    for index in range(limit):
        changed = a[index] != b[index]
        if changed and not in_diff:
            logical_ranges += 1
            in_diff = True
        elif not changed:
            in_diff = False
    if len(a) != len(b) and (limit == 0 or not in_diff):
        logical_ranges += 1
    return ranges, diff_count, logical_ranges > len(ranges)


def compare_bytes(a: bytes, b: bytes) -> dict[str, object]:
    pa = analyze_pe(a)
    pb = analyze_pe(b)
    ranges, diff_count, truncated = _diff_ranges(a, b)
    metadata_union = pa.metadata_ranges + pb.metadata_ranges
    known_only = True
    field_classes: set[str] = set()
    for item in ranges:
        start, end = int(item["start"]), int(item["end"])
        labels = _labels_for_range(start, end, metadata_union)
        item["recognized_metadata"] = labels
        field_classes.update(labels)
        for offset in range(start, min(end, len(a), len(b))):
            if not any(field.start <= offset < field.end for field in metadata_union):
                known_only = False
                break
        if end > min(len(a), len(b)):
            known_only = False

    identical = a == b
    if identical:
        classification = "identical"
    elif len(a) != len(b):
        classification = "layout_or_size_changed"
    elif known_only:
        classification = "recognized_pe_metadata_only"
    else:
        classification = "payload_or_unclassified_bytes_changed"

    section_pairs: list[dict[str, object]] = []
    for index in range(max(len(pa.sections), len(pb.sections))):
        sa = pa.sections[index] if index < len(pa.sections) else None
        sb = pb.sections[index] if index < len(pb.sections) else None
        section_pairs.append(
            {
                "index": index,
                "a": sa,
                "b": sb,
                "same_name": bool(sa and sb and sa["name"] == sb["name"]),
                "same_raw_sha256": bool(sa and sb and sa["raw_sha256"] == sb["raw_sha256"]),
            }
        )

    return {
        "schema_version": 1,
        "comparison": {
            "classification": classification,
            "byte_identical": identical,
            "byte_reproducibility_observed_for_supplied_files": identical,
            "same_size": len(a) == len(b),
            "differing_byte_count": diff_count,
            "diff_range_count_retained": len(ranges),
            "diff_ranges_truncated": truncated,
            "recognized_metadata_only": (not identical and known_only and len(a) == len(b)),
            "changing_field_classes": sorted(field_classes),
            "unclassified_changes_present": (not identical and not known_only),
            "deterministic_linker_contract_established": False,
            "cross_machine_reproducibility_verified": False,
            "independent_acceptance": False,
        },
        "image_a": {
            "sha256": pa.sha256,
            "bytes": pa.size,
            "machine_hex": f"0x{pa.machine:04x}",
            "pe_format": pa.pe_format,
            "coff_timestamp": pa.coff_timestamp,
            "checksum": pa.checksum,
            "reproducible_debug_entry_present": pa.reproducible_debug_entry_present,
            "debug_entries": list(pa.debug_entries),
            "sections": list(pa.sections),
        },
        "image_b": {
            "sha256": pb.sha256,
            "bytes": pb.size,
            "machine_hex": f"0x{pb.machine:04x}",
            "pe_format": pb.pe_format,
            "coff_timestamp": pb.coff_timestamp,
            "checksum": pb.checksum,
            "reproducible_debug_entry_present": pb.reproducible_debug_entry_present,
            "debug_entries": list(pb.debug_entries),
            "sections": list(pb.sections),
        },
        "diff_ranges": ranges,
        "section_comparison": section_pairs,
        "limitations": [
            "Two matching files prove byte identity only for these two supplied builds, not across machines or toolchain updates.",
            "Recognized metadata-only differences localize changed PE fields but do not prove the ambient input that caused them.",
            "A missing IMAGE_DEBUG_TYPE_REPRO entry is not by itself proof that all payload bytes are nondeterministic.",
            "This tool does not alter compiler/linker flags and does not establish Unreal Engine or Unity parity.",
        ],
    }


def compare_files(path_a: Path, path_b: Path) -> dict[str, object]:
    a = path_a.expanduser().resolve(strict=True)
    b = path_b.expanduser().resolve(strict=True)
    if not a.is_file() or not b.is_file():
        raise PEFormatError("both inputs must be regular files")
    if a.stat().st_size > MAX_IMAGE_BYTES or b.stat().st_size > MAX_IMAGE_BYTES:
        raise PEFormatError(f"input exceeds {MAX_IMAGE_BYTES} byte inspection limit")
    report = compare_bytes(a.read_bytes(), b.read_bytes())
    report["input_a"] = str(a)
    report["input_b"] = str(b)
    return report


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Compare two Windows PE images for byte reproducibility.")
    parser.add_argument("image_a", type=Path)
    parser.add_argument("image_b", type=Path)
    parser.add_argument("--json", type=Path, help="Write the report to this path in addition to stdout.")
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    try:
        report = compare_files(args.image_a, args.image_b)
    except (OSError, PEFormatError) as exc:
        print(f"PE reproducibility diagnosis failed: {exc}", file=sys.stderr)
        return 1
    encoded = json.dumps(report, indent=2, sort_keys=True) + "\n"
    print(encoded, end="")
    if args.json is not None:
        args.json.parent.mkdir(parents=True, exist_ok=True)
        args.json.write_text(encoded, encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
