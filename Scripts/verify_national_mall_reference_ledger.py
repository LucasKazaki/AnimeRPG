from __future__ import annotations

import argparse
import json
import math
from pathlib import Path
from urllib.parse import urlparse

STATUS = "reference_validated_not_asset"
RIGHTS_POLICY = "reference_only_no_media_redistribution"
UNITS_POLICY = "retain_source_units_and_convert_to_metres_only_at_authoring_boundary"
ALLOWED_HOSTS = {"www.nps.gov", "www.aoc.gov", "www.si.edu"}
ALLOWED_SOURCE_KINDS = {"official_webpage", "official_pdf"}
ALLOWED_ZONES = {
    "mall_core",
    "west_axis",
    "west_anchor",
    "central_anchor",
    "northwest_landscape",
    "northwest_memorial",
    "east_anchor",
    "museum_edges",
}
ALLOWED_ACCURACY = {"axis_scale", "materials", "landscape", "vegetation", "context_boundary"}
ALLOWED_PRIORITIES = {"A", "B", "C"}
ALLOWED_UNITS = {"count", "feet", "inches", "mile", "acres", "gallons"}
ALLOWED_SOURCE_RELATIONS = {
    "exact_count",
    "exact_source_value",
    "approximate",
    "derived_from_79_ft_10_in",
    "derived_from_555_ft_5_1_8_in",
    "minimum",
}
REQUIRED_COVERAGE = {"axis_scale", "materials", "landscape", "vegetation", "context_boundary"}


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


def load_and_validate(path: Path) -> dict:
    data = json.loads(path.read_text(encoding="utf-8"))
    require(type(data.get("schema_version")) is int and data["schema_version"] == 1, "schema_version")
    require(data.get("loop_id") == "astral-art-hourly-20260922", "loop_id")
    require(data.get("status") == STATUS, "status")
    require(data.get("retrieved_date") == "2026-09-23", "retrieved_date")
    require(data.get("rights_policy") == RIGHTS_POLICY, "rights_policy")
    require(data.get("units_policy") == UNITS_POLICY, "units_policy")
    require(set(data.get("required_coverage", [])) == REQUIRED_COVERAGE, "required_coverage")
    require(isinstance(data.get("review_note"), str) and "No source images" in data["review_note"], "review_note")

    entries = data.get("entries")
    require(isinstance(entries, list) and len(entries) >= 8, "entries")
    seen_ids: set[str] = set()
    coverage: set[str] = set()

    for entry in entries:
        require(isinstance(entry, dict), "entry type")
        entry_id = entry.get("id")
        require(isinstance(entry_id, str) and entry_id and entry_id not in seen_ids, "entry id")
        seen_ids.add(entry_id)
        require(isinstance(entry.get("label"), str) and entry["label"].strip(), f"{entry_id}: label")
        require(entry.get("zone") in ALLOWED_ZONES, f"{entry_id}: zone")
        require(isinstance(entry.get("authority"), str) and entry["authority"].strip(), f"{entry_id}: authority")
        source_kind = entry.get("source_kind")
        require(source_kind in ALLOWED_SOURCE_KINDS, f"{entry_id}: source_kind")
        require(entry.get("rights_mode") == RIGHTS_POLICY, f"{entry_id}: rights_mode")
        require(entry.get("embedded_media") is False, f"{entry_id}: embedded_media")
        require(entry.get("modeling_priority") in ALLOWED_PRIORITIES, f"{entry_id}: priority")
        accuracy = entry.get("accuracy_class")
        require(accuracy in ALLOWED_ACCURACY, f"{entry_id}: accuracy_class")
        coverage.add(accuracy)

        url = entry.get("url")
        require(isinstance(url, str), f"{entry_id}: url")
        parsed = urlparse(url)
        require(parsed.scheme == "https" and parsed.hostname in ALLOWED_HOSTS, f"{entry_id}: authoritative https url")
        require(not parsed.username and not parsed.password and not parsed.fragment, f"{entry_id}: clean url")
        if source_kind == "official_pdf":
            require(parsed.path.lower().endswith(".pdf"), f"{entry_id}: pdf source url")
        else:
            require(not parsed.path.lower().endswith(".pdf"), f"{entry_id}: webpage source url")

        art_use = entry.get("art_use")
        require(isinstance(art_use, list) and len(art_use) >= 2 and all(isinstance(x, str) and x.strip() for x in art_use), f"{entry_id}: art_use")
        facts = entry.get("facts")
        require(isinstance(facts, list) and len(facts) >= 2 and all(isinstance(x, str) and len(x.strip()) >= 20 for x in facts), f"{entry_id}: facts")
        constraints = entry.get("modeling_constraints")
        require(isinstance(constraints, list) and len(constraints) >= 2 and all(isinstance(x, str) and len(x.strip()) >= 20 for x in constraints), f"{entry_id}: constraints")

        measurements = entry.get("measurements")
        require(isinstance(measurements, list), f"{entry_id}: measurements")
        for measurement in measurements:
            require(isinstance(measurement, dict), f"{entry_id}: measurement type")
            require(set(measurement) == {"label", "value", "unit", "source_relation"}, f"{entry_id}: measurement keys")
            require(isinstance(measurement["label"], str) and measurement["label"], f"{entry_id}: measurement label")
            value = measurement["value"]
            require(type(value) in {int, float} and math.isfinite(value) and value > 0, f"{entry_id}: measurement value")
            require(measurement["unit"] in ALLOWED_UNITS, f"{entry_id}: measurement unit")
            require(measurement["source_relation"] in ALLOWED_SOURCE_RELATIONS, f"{entry_id}: measurement provenance")

    require(REQUIRED_COVERAGE.issubset(coverage), "coverage classes")
    return data


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("ledger", type=Path)
    args = parser.parse_args()
    try:
        data = load_and_validate(args.ledger)
    except (OSError, json.JSONDecodeError, ValueError) as exc:
        parser.error(str(exc))
    print(f"PASS: {len(data['entries'])} authoritative National Mall reference entries; source-only")


if __name__ == "__main__":
    main()
