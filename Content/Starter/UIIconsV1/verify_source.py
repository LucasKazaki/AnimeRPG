#!/usr/bin/env python3
"""Verify ART-011 starter UI icon SVG source and expected manifest.

Standard-library only. Source QA only, not Astral runtime acceptance.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import sys
import xml.etree.ElementTree as ET

EXPECTED_IDS = [
    "icon-play", "icon-pause", "icon-stop", "icon-check",
    "icon-close", "icon-warning", "icon-info", "icon-settings",
    "icon-eye", "icon-lock", "icon-search", "icon-menu",
]
XLINK_HREF = "{http://www.w3.org/1999/xlink}href"
PROHIBITED_TAGS = {
    "script", "image", "foreignObject", "animate", "animateMotion",
    "animateTransform", "set", "text", "iframe", "audio", "video",
}
ALLOWED_PAINT = {"currentColor", "none"}
PREVIEW_COLOR = "#000000"
ALLOWED_MANIFEST_STATUSES = {
    "source_prepared_not_imported",
    "source_present_not_independently_validated",
}


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def local_name(tag: str) -> str:
    return tag.rsplit("}", 1)[-1]


def verify(svg_path: Path, manifest_path: Path) -> dict:
    errors: list[str] = []
    checks: list[dict] = []

    def check(name: str, condition: bool, detail: str) -> None:
        checks.append({"name": name, "passed": bool(condition), "detail": detail})
        if not condition:
            errors.append(f"{name}: {detail}")

    svg_bytes = svg_path.read_bytes()
    manifest_bytes = manifest_path.read_bytes()

    try:
        root = ET.fromstring(svg_bytes)
        xml_ok = True
    except ET.ParseError as exc:
        root = None
        xml_ok = False
        errors.append(f"svg_xml_parse: {exc}")
    check("svg_xml_parse", xml_ok, "SVG parses as XML")

    try:
        manifest = json.loads(manifest_bytes)
        manifest_json_ok = isinstance(manifest, dict)
    except Exception as exc:
        manifest = {}
        manifest_json_ok = False
        errors.append(f"manifest_json_parse: {exc}")
    check("manifest_json_parse", manifest_json_ok, "manifest parses as a JSON object")

    svg_hash = sha256_bytes(svg_bytes)
    if manifest_json_ok:
        check("manifest_asset_id", manifest.get("asset_id") == "astral-starter-ui-icons-v1", "asset_id matches ART-011")
        check("manifest_source_file", manifest.get("source_file") == svg_path.name, "manifest source_file matches SVG filename")
        check("manifest_source_sha256", manifest.get("source_sha256") == svg_hash, f"manifest pins SHA-256 {svg_hash}")
        check("manifest_source_bytes", manifest.get("source_bytes") == len(svg_bytes), f"manifest pins byte count {len(svg_bytes)}")
        check("manifest_symbol_count", manifest.get("symbol_count") == 12, "manifest declares 12 symbols")
        check("manifest_contact_count", manifest.get("contact_instance_count") == 12, "manifest declares 12 contact-sheet instances")
        check("manifest_icon_order", manifest.get("icon_ids") == EXPECTED_IDS, "manifest icon_ids exactly match approved order")
        check("manifest_sheet_viewbox", manifest.get("sheet_view_box") == [0, 0, 96, 72], "manifest sheet viewBox is 96x72")
        check("manifest_symbol_viewbox", manifest.get("symbol_view_box") == [0, 0, 24, 24], "manifest symbol viewBox is 24x24")
        check("manifest_runtime_false", manifest.get("runtime_verified") is False, "manifest does not claim runtime verification")
        check("manifest_art_approved_false", manifest.get("art_approved") is False, "manifest does not claim art approval")
        status = manifest.get("status")
        check(
            "manifest_status_not_premature",
            status in ALLOWED_MANIFEST_STATUSES,
            f"status must remain pre-independent-review ({sorted(ALLOWED_MANIFEST_STATUSES)}); observed {status!r}",
        )

    if root is not None:
        check("root_svg", local_name(root.tag) == "svg", "root element is svg")
        check("root_viewbox", root.get("viewBox") == "0 0 96 72", "root viewBox is 0 0 96 72")
        check("root_dimensions", root.get("width") == "96" and root.get("height") == "72", "root width/height are 96x72")

        ids: list[str] = []
        symbols: dict[str, ET.Element] = {}
        uses: list[ET.Element] = []
        prohibited_seen: list[str] = []
        external_hrefs: list[str] = []
        forbidden_paints: list[str] = []
        event_attrs: list[str] = []
        url_attrs: list[str] = []
        hardcoded_symbol_colors: list[str] = []

        for elem in root.iter():
            lname = local_name(elem.tag)
            eid = elem.get("id")
            if eid:
                ids.append(eid)
            if lname == "symbol" and eid:
                symbols[eid] = elem
            if lname == "use":
                uses.append(elem)
            if lname in PROHIBITED_TAGS:
                prohibited_seen.append(lname)
            for key, value in elem.attrib.items():
                kname = local_name(key)
                if kname.lower().startswith("on"):
                    event_attrs.append(f"{lname}.{kname}")
                if "url(" in value.lower():
                    url_attrs.append(f"{lname}.{kname}={value}")
                if kname in {"href", local_name(XLINK_HREF)} and not value.startswith("#"):
                    external_hrefs.append(value)
                if kname in {"fill", "stroke"} and value not in ALLOWED_PAINT:
                    forbidden_paints.append(f"{lname}.{kname}={value}")
                if lname == "symbol" and kname == "color":
                    hardcoded_symbol_colors.append(value)

        check("all_ids_unique", len(ids) == len(set(ids)), "all SVG ids are unique")
        check("symbol_ids_exact", list(symbols.keys()) == EXPECTED_IDS, "symbol ids exactly match approved order")
        check("symbol_count_exact", len(symbols) == 12, "exactly 12 symbols exist")
        check("symbol_viewboxes", all(s.get("viewBox") == "0 0 24 24" for s in symbols.values()), "all symbols use 24x24 viewBox")
        check("contact_use_count", len(uses) == 12, "exactly 12 contact-sheet use elements exist")

        hrefs = [u.get("href") or u.get(XLINK_HREF) for u in uses]
        check("local_use_refs", all(isinstance(h, str) and h.startswith("#") for h in hrefs), "all use hrefs are local fragments")
        check("use_refs_expected_once", hrefs == [f"#{x}" for x in EXPECTED_IDS], "contact sheet references each expected symbol exactly once in order")
        check("no_prohibited_tags", not prohibited_seen, f"no prohibited active/embed/text tags; observed {prohibited_seen}")
        check("no_external_hrefs", not external_hrefs, f"no external hrefs; observed {external_hrefs}")
        check("no_event_handlers", not event_attrs, f"no on* event attributes; observed {event_attrs}")
        check("no_url_references", not url_attrs, f"no CSS/url() references; observed {url_attrs}")
        check("paint_policy", not forbidden_paints, f"fill/stroke use only currentColor or none; observed {forbidden_paints}")
        check("no_symbol_color_override", not hardcoded_symbol_colors, f"symbols do not hard-code color; observed {hardcoded_symbol_colors}")

        wrapper_colors = [e.get("color") for e in root.iter() if e.get("color") is not None]
        check("preview_color_only", wrapper_colors == [PREVIEW_COLOR], f"only wrapper preview color {PREVIEW_COLOR} is present")

    passed = sum(1 for c in checks if c["passed"])
    return {
        "schema_version": 1,
        "tool": "verify_source.py",
        "scope": "source_only_not_runtime_acceptance",
        "svg": {"path": str(svg_path), "bytes": len(svg_bytes), "sha256": svg_hash},
        "manifest": {"path": str(manifest_path), "bytes": len(manifest_bytes), "sha256": sha256_bytes(manifest_bytes)},
        "summary": {"checks": len(checks), "passed": passed, "failed": len(checks) - passed},
        "checks": checks,
        "errors": errors,
    }


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("svg", type=Path)
    ap.add_argument("manifest", type=Path)
    ap.add_argument("--json", dest="json_path", type=Path)
    args = ap.parse_args()
    report = verify(args.svg, args.manifest)
    payload = json.dumps(report, indent=2, sort_keys=True) + "\n"
    if args.json_path:
        args.json_path.write_text(payload, encoding="utf-8")
    else:
        sys.stdout.write(payload)
    return 0 if report["summary"]["failed"] == 0 else 1


if __name__ == "__main__":
    raise SystemExit(main())
