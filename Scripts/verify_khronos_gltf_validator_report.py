#!/usr/bin/env python3
"""Verify a Khronos glTF-Validator JSON report against a specific source asset.

This is an evidence adapter, not a replacement for Khronos glTF-Validator.
It intentionally fails closed on malformed/truncated reports, validator errors,
unexpected warnings, asset/hash mismatches, or external resources.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import sys
from pathlib import Path
from urllib.parse import unquote, urlparse

SEMVER_RE = re.compile(
    r"^(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)"
    r"(?:-[0-9A-Za-z-]+(?:\.[0-9A-Za-z-]+)*)?"
    r"(?:\+[0-9A-Za-z-]+(?:\.[0-9A-Za-z-]+)*)?$"
)
ROOT_KEYS = {"uri", "mimeType", "validatorVersion", "validatedAt", "issues", "info"}
ISSUE_KEYS = {"numErrors", "numWarnings", "numInfos", "numHints", "messages", "truncated"}
MESSAGE_KEYS = {"code", "severity", "pointer", "offset", "message"}
INFO_KEYS = {
    "version", "minVersion", "generator", "extensionsUsed", "extensionsRequired", "resources",
    "animationCount", "materialCount", "hasMorphTargets", "hasSkins", "hasTextures",
    "hasDefaultScene", "drawCallCount", "totalVertexCount", "totalTriangleCount",
    "maxUVs", "maxInfluences", "maxAttributes",
}
INFO_COUNT_FIELDS = {
    "animationCount", "materialCount", "drawCallCount", "totalVertexCount",
    "totalTriangleCount", "maxUVs", "maxInfluences", "maxAttributes",
}
INFO_BOOL_FIELDS = {"hasMorphTargets", "hasSkins", "hasTextures", "hasDefaultScene"}
RESOURCE_KEYS = {"pointer", "storage", "mimeType", "byteLength", "uri", "image"}
RESOURCE_STORAGE = {"data-uri", "buffer-view", "glb", "external"}


class VerificationError(ValueError):
    pass


def _fail(message: str) -> None:
    raise VerificationError(message)


def _strict_int(value: object, label: str, *, minimum: int = 0) -> int:
    if type(value) is not int:
        _fail(f"{label} must be a JSON integer, got {type(value).__name__}")
    if value < minimum:
        _fail(f"{label} must be >= {minimum}, got {value}")
    return value


def _expect_keys(obj: object, allowed: set[str], required: set[str], label: str) -> dict:
    if type(obj) is not dict:
        _fail(f"{label} must be an object")
    unknown = set(obj) - allowed
    missing = required - set(obj)
    if unknown:
        _fail(f"{label} contains unknown fields: {sorted(unknown)}")
    if missing:
        _fail(f"{label} is missing required fields: {sorted(missing)}")
    return obj


def _sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def _resolved_report_path(uri: str) -> Path:
    parsed = urlparse(uri)
    if parsed.scheme not in ("", "file"):
        _fail(f"report.uri uses unsupported scheme {parsed.scheme!r}")
    if parsed.scheme == "file":
        raw = unquote(parsed.path)
        if parsed.netloc:
            raw = f"//{parsed.netloc}{raw}"
    else:
        raw = unquote(uri)
    return Path(raw).resolve(strict=False)


def verify_report(
    report: dict,
    *,
    asset_path: Path,
    expected_sha256: str,
    max_warnings: int = 0,
    require_self_contained: bool = True,
) -> dict:
    root = _expect_keys(report, ROOT_KEYS, {"validatorVersion", "issues"}, "report")

    version = root["validatorVersion"]
    if type(version) is not str or not SEMVER_RE.fullmatch(version):
        _fail("validatorVersion must be a semver string")

    if "uri" not in root or type(root["uri"]) is not str:
        _fail("report.uri must be present and be a string")
    report_asset_path = _resolved_report_path(root["uri"])
    expected_asset_path = asset_path.resolve(strict=False)
    if report_asset_path != expected_asset_path:
        _fail(
            "report.uri does not resolve to the exact asset path: "
            f"report={report_asset_path}, expected={expected_asset_path}"
        )

    if root.get("mimeType") != "model/gltf+json":
        _fail("mimeType must be model/gltf+json for this .gltf asset")

    issues = _expect_keys(root["issues"], ISSUE_KEYS, ISSUE_KEYS, "issues")
    counts = {
        0: _strict_int(issues["numErrors"], "issues.numErrors"),
        1: _strict_int(issues["numWarnings"], "issues.numWarnings"),
        2: _strict_int(issues["numInfos"], "issues.numInfos"),
        3: _strict_int(issues["numHints"], "issues.numHints"),
    }
    if type(issues["truncated"]) is not bool:
        _fail("issues.truncated must be a JSON boolean")
    if issues["truncated"]:
        _fail("validator report is truncated")
    if type(issues["messages"]) is not list:
        _fail("issues.messages must be an array")

    observed = {0: 0, 1: 0, 2: 0, 3: 0}
    for index, raw in enumerate(issues["messages"]):
        message = _expect_keys(raw, MESSAGE_KEYS, {"code", "severity", "message"}, f"issues.messages[{index}]")
        if type(message["code"]) is not str or not message["code"]:
            _fail(f"issues.messages[{index}].code must be a non-empty string")
        severity = _strict_int(message["severity"], f"issues.messages[{index}].severity")
        if severity not in observed:
            _fail(f"issues.messages[{index}].severity must be in 0..3")
        if type(message["message"]) is not str or not message["message"]:
            _fail(f"issues.messages[{index}].message must be a non-empty string")
        has_pointer = "pointer" in message
        has_offset = "offset" in message
        if has_pointer == has_offset:
            _fail(f"issues.messages[{index}] must contain exactly one of pointer or offset")
        if has_pointer and type(message["pointer"]) is not str:
            _fail(f"issues.messages[{index}].pointer must be a string")
        if has_offset:
            _strict_int(message["offset"], f"issues.messages[{index}].offset")
        observed[severity] += 1

    if observed != counts:
        _fail(f"issue summary does not match messages: summary={counts}, messages={observed}")
    if counts[0] != 0:
        _fail(f"Khronos validator reported {counts[0]} error(s)")
    if counts[1] > max_warnings:
        _fail(f"Khronos validator reported {counts[1]} warning(s), limit is {max_warnings}")

    info = _expect_keys(root.get("info"), INFO_KEYS, {"version"}, "info")
    if info["version"] != "2.0":
        _fail(f"info.version must be '2.0', got {info['version']!r}")
    for field in INFO_COUNT_FIELDS:
        if field in info:
            _strict_int(info[field], f"info.{field}")
    for field in INFO_BOOL_FIELDS:
        if field in info and type(info[field]) is not bool:
            _fail(f"info.{field} must be a JSON boolean")
    for field in ("extensionsUsed", "extensionsRequired"):
        if field in info:
            values = info[field]
            if type(values) is not list or not values or any(type(v) is not str or not v for v in values):
                _fail(f"info.{field} must be a non-empty array of strings")
            if len(set(values)) != len(values):
                _fail(f"info.{field} must contain unique strings")
    for field in ("minVersion", "generator"):
        if field in info and type(info[field]) is not str:
            _fail(f"info.{field} must be a string")

    resources_present = "resources" in info
    resources = info.get("resources", [])
    if type(resources) is not list:
        _fail("info.resources must be an array when present")
    if require_self_contained and (not resources_present or not resources):
        _fail("info.resources must be present and non-empty to prove self-contained status")
    for index, raw in enumerate(resources):
        resource = _expect_keys(raw, RESOURCE_KEYS, {"pointer"}, f"info.resources[{index}]")
        if type(resource["pointer"]) is not str:
            _fail(f"info.resources[{index}].pointer must be a string")
        if "storage" not in resource:
            if require_self_contained:
                _fail(f"info.resources[{index}].storage is required to prove self-contained status")
        else:
            storage = resource["storage"]
            if type(storage) is not str or storage not in RESOURCE_STORAGE:
                _fail(f"info.resources[{index}].storage must be a known Khronos storage string")
            if require_self_contained and storage == "external":
                _fail(f"info.resources[{index}] is external; ART-006B must remain self-contained")
        if "byteLength" in resource:
            _strict_int(resource["byteLength"], f"info.resources[{index}].byteLength", minimum=1)
        if "mimeType" in resource and type(resource["mimeType"]) is not str:
            _fail(f"info.resources[{index}].mimeType must be a string")
        if "uri" in resource and type(resource["uri"]) is not str:
            _fail(f"info.resources[{index}].uri must be a string")
        if "image" in resource and type(resource["image"]) is not dict:
            _fail(f"info.resources[{index}].image must be an object")

    if not asset_path.is_file():
        _fail(f"asset does not exist: {asset_path}")
    actual_sha = _sha256(asset_path)
    normalized_expected = expected_sha256.lower()
    if not re.fullmatch(r"[0-9a-f]{64}", normalized_expected):
        _fail("expected_sha256 must be 64 lowercase/uppercase hex characters")
    if actual_sha != normalized_expected:
        _fail(f"asset SHA-256 mismatch: expected {normalized_expected}, got {actual_sha}")

    return {
        "asset": asset_path.name,
        "asset_sha256": actual_sha,
        "validator_version": version,
        "errors": counts[0],
        "warnings": counts[1],
        "infos": counts[2],
        "hints": counts[3],
        "self_contained": require_self_contained,
        "status": "khronos_report_accepted_source_only",
    }


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--asset", type=Path, required=True)
    parser.add_argument("--report", type=Path, required=True)
    parser.add_argument("--expected-sha256", required=True)
    parser.add_argument("--max-warnings", type=int, default=0)
    parser.add_argument("--allow-external-resources", action="store_true")
    args = parser.parse_args(argv)

    if args.max_warnings < 0:
        parser.error("--max-warnings must be >= 0")

    try:
        report = json.loads(args.report.read_text(encoding="utf-8"))
        receipt = verify_report(
            report,
            asset_path=args.asset,
            expected_sha256=args.expected_sha256,
            max_warnings=args.max_warnings,
            require_self_contained=not args.allow_external_resources,
        )
    except (OSError, json.JSONDecodeError, VerificationError) as exc:
        print(f"FAIL: {exc}", file=sys.stderr)
        return 1

    print("PASS: " + json.dumps(receipt, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
