#!/usr/bin/env python3
"""Inspect a Windows PE image's imported DLL dependencies without third-party packages.

This is a packaging evidence tool, not a clean-machine compatibility oracle. It reads
normal and delay-load import tables, reports the imported DLL names, and can reject
Debug CRT dependencies that must never ship in a Release package.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import struct
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable

MAX_IMAGE_BYTES = 512 * 1024 * 1024
MAX_IMPORT_DESCRIPTORS = 16384
MAX_DLL_NAME_BYTES = 1024

MACHINE_NAMES = {
    0x014C: "I386",
    0x8664: "AMD64",
    0xAA64: "ARM64",
}

_DEBUG_CRT = re.compile(
    r"^(?:msvcp\d+(?:_\d+)?d|vcruntime\d+(?:_\d+)?d|concrt\d+d|msvcr\d+d|vcomp\d+d|ucrtbased)\.dll$",
    re.IGNORECASE,
)
_VC_RUNTIME = re.compile(
    r"^(?:msvcp\d+(?:_\d+)?|vcruntime\d+(?:_\d+)?|concrt\d+|msvcr\d+|vcomp\d+)\.dll$",
    re.IGNORECASE,
)
_UCRT = re.compile(r"^(?:ucrtbase|api-ms-win-crt-[a-z0-9-]+)\.dll$", re.IGNORECASE)
_API_SET = re.compile(r"^(?:api-ms-win-|ext-ms-win-).+\.dll$", re.IGNORECASE)


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
class PEImports:
    machine: str
    machine_hex: str
    format: str
    image_base: int
    imports: tuple[str, ...]
    delay_imports: tuple[str, ...]

    @property
    def all_imports(self) -> tuple[str, ...]:
        return tuple(sorted(set(self.imports) | set(self.delay_imports), key=str.lower))


def _require_range(data: bytes, offset: int, size: int, label: str) -> None:
    if offset < 0 or size < 0 or offset > len(data) or size > len(data) - offset:
        raise PEFormatError(f"{label} extends outside file bounds")


def _u16(data: bytes, offset: int, label: str) -> int:
    _require_range(data, offset, 2, label)
    return struct.unpack_from("<H", data, offset)[0]


def _u32(data: bytes, offset: int, label: str) -> int:
    _require_range(data, offset, 4, label)
    return struct.unpack_from("<I", data, offset)[0]


def _u64(data: bytes, offset: int, label: str) -> int:
    _require_range(data, offset, 8, label)
    return struct.unpack_from("<Q", data, offset)[0]


def _decode_section_name(raw: bytes) -> str:
    return raw.split(b"\0", 1)[0].decode("ascii", errors="replace")


def _read_c_string(data: bytes, offset: int, label: str) -> str:
    _require_range(data, offset, 1, label)
    limit = min(len(data), offset + MAX_DLL_NAME_BYTES)
    terminator = data.find(b"\0", offset, limit)
    if terminator < 0:
        raise PEFormatError(f"{label} is not NUL terminated within {MAX_DLL_NAME_BYTES} bytes")
    raw = data[offset:terminator]
    if not raw:
        raise PEFormatError(f"{label} is empty")
    try:
        return raw.decode("ascii")
    except UnicodeDecodeError as exc:
        raise PEFormatError(f"{label} is not ASCII") from exc


def _rva_to_offset(rva: int, sections: Iterable[Section], data_size: int, label: str) -> int:
    if rva == 0:
        raise PEFormatError(f"{label} has a null RVA")
    for section in sections:
        span = max(section.virtual_size, section.raw_size)
        if section.virtual_address <= rva < section.virtual_address + span:
            delta = rva - section.virtual_address
            if delta >= section.raw_size:
                raise PEFormatError(f"{label} points into virtual-only section data")
            offset = section.raw_offset + delta
            if offset < 0 or offset >= data_size:
                raise PEFormatError(f"{label} resolves outside file bounds")
            return offset
    raise PEFormatError(f"{label} RVA 0x{rva:x} is not mapped by a section")


def _parse_import_directory(
    data: bytes,
    directory_rva: int,
    directory_size: int,
    sections: tuple[Section, ...],
) -> tuple[str, ...]:
    if directory_rva == 0 or directory_size == 0:
        return ()
    start = _rva_to_offset(directory_rva, sections, len(data), "import directory")
    names: list[str] = []
    descriptor_size = 20
    limit = min(MAX_IMPORT_DESCRIPTORS, max(1, directory_size // descriptor_size + 1))
    terminated = False
    for index in range(limit):
        offset = start + index * descriptor_size
        _require_range(data, offset, descriptor_size, "import descriptor")
        original_first_thunk, timestamp, forwarder_chain, name_rva, first_thunk = struct.unpack_from(
            "<IIIII", data, offset
        )
        if not any((original_first_thunk, timestamp, forwarder_chain, name_rva, first_thunk)):
            terminated = True
            break
        name_offset = _rva_to_offset(name_rva, sections, len(data), "import DLL name")
        names.append(_read_c_string(data, name_offset, "import DLL name"))
    if not terminated:
        raise PEFormatError("import descriptor table has no bounded terminator")
    return tuple(sorted(set(names), key=str.lower))


def _parse_delay_import_directory(
    data: bytes,
    directory_rva: int,
    directory_size: int,
    sections: tuple[Section, ...],
    image_base: int,
) -> tuple[str, ...]:
    if directory_rva == 0 or directory_size == 0:
        return ()
    start = _rva_to_offset(directory_rva, sections, len(data), "delay import directory")
    names: list[str] = []
    descriptor_size = 32
    limit = min(MAX_IMPORT_DESCRIPTORS, max(1, directory_size // descriptor_size + 1))
    terminated = False
    for index in range(limit):
        offset = start + index * descriptor_size
        _require_range(data, offset, descriptor_size, "delay import descriptor")
        fields = struct.unpack_from("<IIIIIIII", data, offset)
        if not any(fields):
            terminated = True
            break
        attributes, name_value = fields[0], fields[1]
        if name_value == 0:
            raise PEFormatError("delay import descriptor has a null DLL name")
        if attributes & 1:
            name_rva = name_value
        else:
            if name_value < image_base:
                raise PEFormatError("delay import DLL name VA precedes image base")
            name_rva = name_value - image_base
        name_offset = _rva_to_offset(name_rva, sections, len(data), "delay import DLL name")
        names.append(_read_c_string(data, name_offset, "delay import DLL name"))
    if not terminated:
        raise PEFormatError("delay import descriptor table has no bounded terminator")
    return tuple(sorted(set(names), key=str.lower))


def parse_pe_imports(data: bytes) -> PEImports:
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
    optional_size = _u16(data, coff + 16, "COFF optional-header size")
    if section_count == 0 or section_count > 96:
        raise PEFormatError(f"unreasonable section count: {section_count}")

    optional = coff + 20
    _require_range(data, optional, optional_size, "optional header")
    magic = _u16(data, optional, "optional-header magic")
    if magic == 0x10B:
        pe_format = "PE32"
        image_base = _u32(data, optional + 28, "PE32 image base")
        number_rvas_offset = 92
        data_directory_offset = 96
    elif magic == 0x20B:
        pe_format = "PE32+"
        image_base = _u64(data, optional + 24, "PE32+ image base")
        number_rvas_offset = 108
        data_directory_offset = 112
    else:
        raise PEFormatError(f"unsupported optional-header magic 0x{magic:04x}")

    if optional_size < data_directory_offset:
        raise PEFormatError("optional header does not contain a data-directory table")
    number_rvas = _u32(data, optional + number_rvas_offset, "number of data directories")

    def directory(index: int) -> tuple[int, int]:
        if number_rvas <= index:
            return (0, 0)
        relative = data_directory_offset + index * 8
        if relative + 8 > optional_size:
            raise PEFormatError("declared data-directory count exceeds optional header")
        return (
            _u32(data, optional + relative, f"data directory {index} RVA"),
            _u32(data, optional + relative + 4, f"data directory {index} size"),
        )

    section_table = optional + optional_size
    _require_range(data, section_table, section_count * 40, "section table")
    sections: list[Section] = []
    for index in range(section_count):
        offset = section_table + index * 40
        name = _decode_section_name(data[offset : offset + 8])
        virtual_size = _u32(data, offset + 8, "section virtual size")
        virtual_address = _u32(data, offset + 12, "section virtual address")
        raw_size = _u32(data, offset + 16, "section raw size")
        raw_offset = _u32(data, offset + 20, "section raw offset")
        if raw_size:
            _require_range(data, raw_offset, raw_size, f"section {name or index} raw data")
        sections.append(Section(name, virtual_address, virtual_size, raw_offset, raw_size))
    section_tuple = tuple(sections)

    import_rva, import_size = directory(1)
    delay_rva, delay_size = directory(13)
    imports = _parse_import_directory(data, import_rva, import_size, section_tuple)
    delay_imports = _parse_delay_import_directory(
        data, delay_rva, delay_size, section_tuple, image_base
    )
    return PEImports(
        machine=MACHINE_NAMES.get(machine, "UNKNOWN"),
        machine_hex=f"0x{machine:04x}",
        format=pe_format,
        image_base=image_base,
        imports=imports,
        delay_imports=delay_imports,
    )


def inspect_file(path: Path) -> dict[str, object]:
    resolved = path.expanduser().resolve(strict=True)
    if not resolved.is_file():
        raise PEFormatError(f"not a regular file: {resolved}")
    size = resolved.stat().st_size
    if size > MAX_IMAGE_BYTES:
        raise PEFormatError(f"file exceeds {MAX_IMAGE_BYTES} byte inspection limit")
    data = resolved.read_bytes()
    parsed = parse_pe_imports(data)
    all_imports = parsed.all_imports
    debug_crt = [name for name in all_imports if _DEBUG_CRT.match(name)]
    vc_runtime = [name for name in all_imports if _VC_RUNTIME.match(name) and name not in debug_crt]
    ucrt = [name for name in all_imports if _UCRT.match(name)]
    api_sets = [name for name in all_imports if _API_SET.match(name)]
    return {
        "schema_version": 1,
        "file": str(resolved),
        "bytes": len(data),
        "sha256": hashlib.sha256(data).hexdigest(),
        "machine": parsed.machine,
        "machine_hex": parsed.machine_hex,
        "pe_format": parsed.format,
        "image_base": parsed.image_base,
        "imports": list(parsed.imports),
        "delay_imports": list(parsed.delay_imports),
        "all_imports": list(all_imports),
        "analysis": {
            "debug_crt_imports": debug_crt,
            "vc_runtime_imports": vc_runtime,
            "ucrt_imports": ucrt,
            "api_set_imports": api_sets,
            "requires_vc_runtime_deployment_review": bool(vc_runtime),
            "clean_machine_compatibility_verified": False,
        },
        "limitations": [
            "Import-table inspection does not prove that a DLL is installed on a target machine.",
            "This report does not replace launching the package on a supported clean Windows machine.",
            "Native plugins loaded dynamically with LoadLibrary are not discoverable from PE import tables alone.",
        ],
    }


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Inspect imported DLL dependencies of a Windows PE image.")
    parser.add_argument("image", type=Path)
    parser.add_argument("--json", type=Path, help="Write the report to this path in addition to stdout.")
    parser.add_argument(
        "--fail-on-debug-runtime",
        action="store_true",
        help="Return exit 2 if the image imports a known Debug Microsoft C/C++ runtime DLL.",
    )
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    try:
        report = inspect_file(args.image)
    except (OSError, PEFormatError) as exc:
        print(f"PE dependency inspection failed: {exc}", file=sys.stderr)
        return 1
    encoded = json.dumps(report, indent=2, sort_keys=True) + "\n"
    print(encoded, end="")
    if args.json is not None:
        args.json.parent.mkdir(parents=True, exist_ok=True)
        args.json.write_text(encoded, encoding="utf-8")
    if args.fail_on_debug_runtime and report["analysis"]["debug_crt_imports"]:
        print("Release image imports a Debug Microsoft C/C++ runtime DLL.", file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
