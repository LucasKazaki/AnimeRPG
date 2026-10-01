#!/usr/bin/env python3
import argparse
import hashlib
import json
from pathlib import Path

EXPECTED_VIEWS = {"front": 0, "three_quarter": 45, "profile": 90, "rear": 180}
EXPECTED_ARCHETYPES = {"Shadowblade", "Arc Mage", "Aegis"}
EXPECTED_FLAGS = {
    "source_contract_validated",
    "representative_runtime_fixture_captured",
    "independent_visual_reviewed",
    "production_topology_uv_rig_validated",
    "art_approved",
    "ue_unity_parity_claim",
}

def load_json(path):
    seen = {}
    def hook(pairs):
        out = {}
        for key, value in pairs:
            if key in out:
                raise ValueError("duplicate JSON key: " + key)
            out[key] = value
        return out
    raw = Path(path).read_bytes()
    return json.loads(raw.decode("utf-8"), object_pairs_hook=hook), raw

def validate(doc, raw, expected_sha=None):
    checks = []
    def check(name, ok):
        checks.append((name, bool(ok)))

    digest = hashlib.sha256(raw).hexdigest()
    check("sha256", expected_sha is None or digest == expected_sha)
    check("schema", doc.get("schema_version") == 1)
    check("task", doc.get("task_id") == "ART-012")
    check("loop", doc.get("loop_id") == "astral-art-hourly-20260922")
    check("status", doc.get("status") == "proposed_source_art_direction_contract_not_runtime_accepted")

    baseline = doc.get("baseline", {})
    check("baseline", baseline.get("main_commit") == "c644eec79752b9dc101aeca6b03141cf26854c96")
    check("fixture_not_production", "not a production protagonist" in baseline.get("claim_boundary", ""))

    identity = doc.get("product_identity", {})
    check("one_protagonist", "one persistent customizable protagonist" in identity.get("protagonist_policy", ""))
    check("identity_archetypes", set(identity.get("archetypes", [])) == EXPECTED_ARCHETYPES)
    check("protected_reference_boundary", "no protected" in identity.get("protected_reference_policy", "").lower())

    fixture = doc.get("review_fixture", {})
    check("body_heights", fixture.get("body_height_px") == [64, 96, 128])
    check("primary_height", fixture.get("primary_body_height_px") == 96)
    views = {x.get("name"): x.get("yaw_deg") for x in fixture.get("views", []) if isinstance(x, dict)}
    check("views", views == EXPECTED_VIEWS)
    backgrounds = fixture.get("background_relative_luminance", [])
    check("backgrounds", len(backgrounds) == 3 and len(set(backgrounds)) == 3 and all(0 <= x <= 1 for x in backgrounds))
    check("vfx_off", fixture.get("vfx_for_base_review") == "off")

    silhouette = doc.get("silhouette_rules", {})
    check("negative_space", silhouette.get("proposed_min_negative_space_px_at_96") == 2)
    failure = silhouette.get("failure_rule", "")
    check("effect_independent", all(x in failure for x in ["glow", "hue", "particles", "UI labels", "camera motion"]))

    variants = doc.get("anatomy_and_variant_rules", {})
    check("same_protagonist", "same protagonist" in variants.get("shared_identity", ""))
    check("rig_gate", "actual accepted rig" in variants.get("deformation_gate", ""))

    archetypes = doc.get("archetype_readability", {})
    check("archetype_keys", set(archetypes) == EXPECTED_ARCHETYPES)
    check("archetype_fields", all(
        isinstance(archetypes.get(name), dict)
        and all(archetypes[name].get(field) for field in ["shape_language", "primary_equipment", "pose_read"])
        for name in EXPECTED_ARCHETYPES
    ))

    toon = doc.get("toon_lighting_and_effect_rules", {})
    check("rim_off_gate", "rim disabled" in toon.get("rim", ""))
    check("outline_ab", "outline-on and outline-off" in toon.get("outline", ""))

    stages = doc.get("acceptance_stages", {})
    check("flags", set(stages) == EXPECTED_FLAGS)
    check("flags_false", all(stages.get(x) is False for x in EXPECTED_FLAGS))

    captures = "\n".join(doc.get("runtime_capture_requirements", []))
    check("astral_capture", "current Astral renderer" in captures and "not offline Blender" in captures)
    check("capture_grid", "all four review views" in captures and "64/96/128 px" in captures)
    check("capture_metadata", all(x in captures for x in ["exact commit", "resolution", "FOV", "camera transform", "capture hash"]))
    check("independent_review", "independent reviewer" in captures)

    failed = [name for name, passed in checks if not passed]
    return {
        "task_id": "ART-012",
        "contract_sha256": digest,
        "checks_total": len(checks),
        "checks_passed": len(checks) - len(failed),
        "checks_failed": len(failed),
        "failed": failed,
        "passed": not failed,
        "claim_boundary": "source semantics only; no runtime, rig, performance, independent visual, art approval, or engine parity claim",
    }

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("contract", nargs="?", default=str(Path(__file__).with_name("protagonist-readability-contract.json")))
    parser.add_argument("--expected-sha256")
    args = parser.parse_args()
    try:
        doc, raw = load_json(args.contract)
        result = validate(doc, raw, args.expected_sha256)
    except Exception as exc:
        print(json.dumps({"task_id": "ART-012", "passed": False, "error": str(exc)}, indent=2))
        return 2
    print(json.dumps(result, indent=2, sort_keys=True))
    return 0 if result["passed"] else 1

if __name__ == "__main__":
    raise SystemExit(main())
