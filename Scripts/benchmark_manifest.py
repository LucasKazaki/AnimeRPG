#!/usr/bin/env python3
from __future__ import annotations

import argparse, hashlib, json, os, re, sys
from datetime import datetime, timezone
from pathlib import Path, PurePosixPath
from typing import Any
import release_manifest

SCHEMA_VERSION = 1
MAX_SPEC_BYTES = 1024 * 1024
MAX_MANIFEST_BYTES = 4 * 1024 * 1024
MAX_EVIDENCE_FILES = 256
MAX_EVIDENCE_FILE_BYTES = 2 * 1024 * 1024 * 1024
SHA_RE = re.compile(r"^[0-9a-fA-F]{64}$")
COMMIT_RE = re.compile(r"^[0-9a-fA-F]{40}$")
ID_RE = re.compile(r"^[A-Za-z0-9_.-]{1,128}$")
ROLE_RE = re.compile(r"^[a-z0-9_.-]{1,64}$")
UTC_RE = re.compile(r"^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}Z$")

ACCEPTANCE = {
    "performance_budget_verified": False,
    "comparative_parity_verified": False,
    "clean_machine_compatibility_verified": False,
    "independent_acceptance": False,
}
LIMITATIONS = [
    "This manifest binds benchmark protocol, environment labels, package bytes, and evidence-file hashes; it does not measure frame time or resource usage itself.",
    "Environment fields are operator-supplied labels and require retained machine receipts for independent verification.",
    "Reference engine versions are comparison targets only; this manifest does not prove matched Unreal Engine or Unity runs occurred.",
    "No performance budget, comparative parity, clean-machine compatibility, or independent acceptance follows from manifest creation or verification.",
]


class BenchmarkManifestError(ValueError):
    pass


def _sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def _load_json(path: Path, limit: int, label: str) -> dict[str, Any]:
    try:
        if path.stat().st_size > limit:
            raise BenchmarkManifestError(f"{label} byte limit exceeded")
        data = json.loads(path.read_text(encoding="utf-8"))
    except BenchmarkManifestError:
        raise
    except Exception as exc:
        raise BenchmarkManifestError(f"cannot read {label}: {exc}") from exc
    if not isinstance(data, dict):
        raise BenchmarkManifestError(f"{label} root must be an object")
    return data


def _keys(obj: Any, expected: set[str], label: str) -> dict[str, Any]:
    if not isinstance(obj, dict):
        raise BenchmarkManifestError(f"{label} must be an object")
    if set(obj) != expected:
        raise BenchmarkManifestError(
            f"{label} keys mismatch; missing={sorted(expected-set(obj))}, extra={sorted(set(obj)-expected)}"
        )
    return obj


def _text(value: Any, label: str, max_len: int = 512) -> str:
    if not isinstance(value, str) or not value.strip() or len(value) > max_len:
        raise BenchmarkManifestError(f"{label} must be non-empty text <= {max_len} characters")
    return value


def _number(value: Any, label: str, low: float, high: float, allow_zero: bool) -> float:
    if isinstance(value, bool) or not isinstance(value, (int, float)):
        raise BenchmarkManifestError(f"{label} must be numeric")
    value = float(value)
    if not (value == value and abs(value) != float("inf")):
        raise BenchmarkManifestError(f"{label} must be finite")
    if value < low or value > high or (not allow_zero and value == 0):
        raise BenchmarkManifestError(f"{label} is out of range")
    return value


def _safe_rel(value: Any) -> str:
    if not isinstance(value, str) or not value or "\x00" in value or "\\" in value:
        raise BenchmarkManifestError("evidence paths must be non-empty forward-slash relative paths")
    if ":" in value.split("/", 1)[0]:
        raise BenchmarkManifestError(f"drive-qualified evidence path: {value!r}")
    p = PurePosixPath(value)
    if p.is_absolute() or any(part in ("", ".", "..") for part in p.parts) or p.as_posix() != value:
        raise BenchmarkManifestError(f"unsafe evidence path: {value!r}")
    return value


def validate_spec(spec: dict[str, Any]) -> dict[str, Any]:
    _keys(spec, {"candidate","workload","run_protocol","environment","reference_versions","provenance","evidence"}, "spec")
    candidate = _keys(spec["candidate"], {"commit","build_config"}, "candidate")
    if not isinstance(candidate["commit"], str) or not COMMIT_RE.fullmatch(candidate["commit"]):
        raise BenchmarkManifestError("candidate.commit must be 40 hex characters")
    if candidate["build_config"] not in ("Release", "RelWithDebInfo"):
        raise BenchmarkManifestError("candidate.build_config must be Release or RelWithDebInfo")

    workload = _keys(spec["workload"], {"id","dimension","fixture_description"}, "workload")
    if not isinstance(workload["id"], str) or not ID_RE.fullmatch(workload["id"]):
        raise BenchmarkManifestError("workload.id is invalid")
    if workload["dimension"] not in ("2d", "3d"):
        raise BenchmarkManifestError("workload.dimension must be 2d or 3d")
    _text(workload["fixture_description"], "workload.fixture_description")

    protocol = _keys(spec["run_protocol"], {"width","height","window_mode","vsync","warmup_seconds","sample_seconds"}, "run_protocol")
    for key in ("width", "height"):
        if isinstance(protocol[key], bool) or not isinstance(protocol[key], int) or not 1 <= protocol[key] <= 16384:
            raise BenchmarkManifestError(f"run_protocol.{key} must be an integer in [1, 16384]")
    if protocol["window_mode"] not in ("windowed", "borderless", "fullscreen"):
        raise BenchmarkManifestError("run_protocol.window_mode is invalid")
    if not isinstance(protocol["vsync"], bool):
        raise BenchmarkManifestError("run_protocol.vsync must be boolean")
    _number(protocol["warmup_seconds"], "run_protocol.warmup_seconds", 0, 3600, True)
    _number(protocol["sample_seconds"], "run_protocol.sample_seconds", 0, 86400, False)

    env = _keys(spec["environment"], {"os","cpu","logical_cpus","ram_bytes","gpu","gpu_driver"}, "environment")
    for key in ("os", "cpu", "gpu", "gpu_driver"):
        _text(env[key], f"environment.{key}", 256)
    if isinstance(env["logical_cpus"], bool) or not isinstance(env["logical_cpus"], int) or not 1 <= env["logical_cpus"] <= 1024:
        raise BenchmarkManifestError("environment.logical_cpus must be an integer in [1, 1024]")
    if isinstance(env["ram_bytes"], bool) or not isinstance(env["ram_bytes"], int) or env["ram_bytes"] <= 0:
        raise BenchmarkManifestError("environment.ram_bytes must be a positive integer")

    refs = _keys(spec["reference_versions"], {"unreal","unity"}, "reference_versions")
    _text(refs["unreal"], "reference_versions.unreal", 64)
    _text(refs["unity"], "reference_versions.unity", 64)

    provenance = _keys(spec["provenance"], {"evidence_class","machine_label"}, "provenance")
    if provenance["evidence_class"] not in ("local_native", "hosted_contract_fixture"):
        raise BenchmarkManifestError("provenance.evidence_class is invalid")
    _text(provenance["machine_label"], "provenance.machine_label", 128)

    evidence = spec["evidence"]
    if not isinstance(evidence, list) or not 1 <= len(evidence) <= MAX_EVIDENCE_FILES:
        raise BenchmarkManifestError(f"evidence must contain 1..{MAX_EVIDENCE_FILES} entries")
    seen = set()
    clean = []
    for entry in evidence:
        entry = _keys(entry, {"path","role"}, "evidence entry")
        rel, role = _safe_rel(entry["path"]), entry["role"]
        if not isinstance(role, str) or not ROLE_RE.fullmatch(role):
            raise BenchmarkManifestError("evidence role is invalid")
        if rel.casefold() in seen:
            raise BenchmarkManifestError(f"duplicate or Windows-case-colliding evidence path: {rel}")
        seen.add(rel.casefold())
        clean.append({"path": rel, "role": role})

    out = json.loads(json.dumps(spec))
    out["candidate"]["commit"] = candidate["commit"].lower()
    out["evidence"] = clean
    return out


def _evidence_file(root: Path, rel: str) -> Path:
    if root.is_symlink():
        raise BenchmarkManifestError("evidence root must not be a symlink")
    root = root.resolve(strict=True)
    if not root.is_dir():
        raise BenchmarkManifestError("evidence root must be a directory")
    p = root
    for part in PurePosixPath(rel).parts:
        p = p / part
        if p.is_symlink():
            raise BenchmarkManifestError(f"symlinked evidence path component: {rel}")
    try:
        resolved = p.resolve(strict=True)
        if os.path.commonpath((str(root), str(resolved))) != str(root):
            raise BenchmarkManifestError(f"evidence escapes root: {rel}")
    except ValueError as exc:
        raise BenchmarkManifestError(f"evidence escapes root: {rel}") from exc
    if not resolved.is_file():
        raise BenchmarkManifestError(f"evidence is not a regular file: {rel}")
    if resolved.stat().st_size > MAX_EVIDENCE_FILE_BYTES:
        raise BenchmarkManifestError(f"evidence file byte limit exceeded: {rel}")
    return resolved


def _release_binding(manifest_path: Path, package_root: Path, commit: str) -> tuple[str, str, int]:
    release = release_manifest.load_manifest(manifest_path)
    exe_hash = next((i.get("sha256") for i in release.get("files", []) if isinstance(i, dict) and i.get("path") == "AstralGame.exe"), None)
    if not isinstance(exe_hash, str) or not SHA_RE.fullmatch(exe_hash):
        raise BenchmarkManifestError("release manifest lacks a valid AstralGame.exe hash")
    try:
        report = release_manifest.verify_manifest(
            release, package_root, manifest_path=manifest_path,
            expected_commit=commit, expected_executable_sha256=exe_hash,
        )
    except (release_manifest.ManifestError, OSError) as exc:
        raise BenchmarkManifestError(f"release package verification failed: {exc}") from exc
    return _sha256(manifest_path), exe_hash.lower(), int(report["files_verified"])


def _descriptor_hash(value: dict[str, Any]) -> str:
    return hashlib.sha256(json.dumps(value, sort_keys=True, separators=(",", ":")).encode()).hexdigest()


def build_manifest(spec: dict[str, Any], package_root: Path, release_path: Path, evidence_root: Path) -> dict[str, Any]:
    spec = validate_spec(spec)
    release_hash, exe_hash, file_count = _release_binding(release_path, package_root, spec["candidate"]["commit"])
    evidence = []
    for item in spec["evidence"]:
        p = _evidence_file(evidence_root, item["path"])
        evidence.append({**item, "bytes": p.stat().st_size, "sha256": _sha256(p)})
    descriptor = {
        "candidate": {**spec["candidate"], "release_manifest_sha256": release_hash, "executable_sha256": exe_hash, "package_files_verified": file_count},
        "workload": spec["workload"], "run_protocol": spec["run_protocol"],
        "environment": spec["environment"], "reference_versions": spec["reference_versions"],
        "provenance": spec["provenance"], "evidence": evidence,
    }
    return {
        "schema_version": SCHEMA_VERSION,
        "generated_at_utc": datetime.now(timezone.utc).isoformat(timespec="seconds").replace("+00:00", "Z"),
        **descriptor,
        "benchmark_descriptor_sha256": _descriptor_hash(descriptor),
        "acceptance": dict(ACCEPTANCE), "limitations": list(LIMITATIONS),
    }


def verify_manifest(manifest: dict[str, Any], package_root: Path, release_path: Path, evidence_root: Path) -> dict[str, Any]:
    top = {"schema_version","generated_at_utc","candidate","workload","run_protocol","environment","reference_versions","provenance","evidence","benchmark_descriptor_sha256","acceptance","limitations"}
    if not isinstance(manifest, dict) or set(manifest) != top:
        raise BenchmarkManifestError("benchmark manifest shape/schema mismatch")
    if isinstance(manifest.get("schema_version"), bool) or manifest.get("schema_version") != SCHEMA_VERSION:
        raise BenchmarkManifestError("benchmark manifest shape/schema mismatch")
    if not isinstance(manifest.get("generated_at_utc"), str) or not UTC_RE.fullmatch(manifest["generated_at_utc"]):
        raise BenchmarkManifestError("generated_at_utc must be second-resolution UTC RFC3339 text")
    acceptance = _keys(manifest.get("acceptance"), set(ACCEPTANCE), "acceptance")
    if any(acceptance[key] is not False for key in ACCEPTANCE) or manifest.get("limitations") != LIMITATIONS:
        raise BenchmarkManifestError("benchmark manifest claim boundaries were changed")
    candidate = _keys(manifest["candidate"], {"commit","build_config","release_manifest_sha256","executable_sha256","package_files_verified"}, "candidate")
    for key in ("release_manifest_sha256", "executable_sha256"):
        if not isinstance(candidate[key], str) or not SHA_RE.fullmatch(candidate[key]):
            raise BenchmarkManifestError(f"candidate.{key} must be a SHA-256 digest")
    if (isinstance(candidate["package_files_verified"], bool)
            or not isinstance(candidate["package_files_verified"], int)
            or candidate["package_files_verified"] < 0):
        raise BenchmarkManifestError("candidate.package_files_verified must be a non-negative integer")
    evidence = manifest.get("evidence")
    if not isinstance(evidence, list):
        raise BenchmarkManifestError("benchmark evidence must be an array")
    spec = {
        "candidate": {"commit": candidate["commit"], "build_config": candidate["build_config"]},
        "workload": manifest["workload"], "run_protocol": manifest["run_protocol"],
        "environment": manifest["environment"], "reference_versions": manifest["reference_versions"],
        "provenance": manifest["provenance"],
        "evidence": [{"path": i.get("path"), "role": i.get("role")} if isinstance(i, dict) else i for i in evidence],
    }
    spec = validate_spec(spec)
    release_hash, exe_hash, file_count = _release_binding(release_path, package_root, spec["candidate"]["commit"])
    if candidate["release_manifest_sha256"] != release_hash or candidate["executable_sha256"] != exe_hash or candidate["package_files_verified"] != file_count:
        raise BenchmarkManifestError("candidate package binding mismatch")
    checked = []
    for item in evidence:
        item = _keys(item, {"path","role","bytes","sha256"}, "benchmark evidence entry")
        if isinstance(item["bytes"], bool) or not isinstance(item["bytes"], int) or item["bytes"] < 0:
            raise BenchmarkManifestError(f"invalid evidence byte count: {item['path']}")
        if not isinstance(item["sha256"], str) or not SHA_RE.fullmatch(item["sha256"]):
            raise BenchmarkManifestError(f"invalid evidence hash: {item['path']}")
        p = _evidence_file(evidence_root, _safe_rel(item["path"]))
        size, digest = p.stat().st_size, _sha256(p)
        if item["bytes"] != size or item["sha256"] != digest:
            raise BenchmarkManifestError(f"evidence changed: {item['path']}")
        checked.append({"path": item["path"], "role": item["role"], "bytes": size, "sha256": digest})
    descriptor = {
        "candidate": {**spec["candidate"], "release_manifest_sha256": release_hash, "executable_sha256": exe_hash, "package_files_verified": file_count},
        "workload": spec["workload"], "run_protocol": spec["run_protocol"],
        "environment": spec["environment"], "reference_versions": spec["reference_versions"],
        "provenance": spec["provenance"], "evidence": checked,
    }
    digest = manifest.get("benchmark_descriptor_sha256")
    if not isinstance(digest, str) or not SHA_RE.fullmatch(digest) or digest.lower() != _descriptor_hash(descriptor):
        raise BenchmarkManifestError("benchmark descriptor hash mismatch")
    return {
        "schema_version": 1, "benchmark_manifest_verified": True,
        "candidate_commit": spec["candidate"]["commit"], "release_manifest_sha256": release_hash,
        "executable_sha256": exe_hash, "evidence_files_verified": len(checked),
        "benchmark_descriptor_sha256": digest.lower(), "acceptance": dict(ACCEPTANCE),
        "limitations": [
            "Verification proves manifest/package/evidence consistency only; it does not validate operator-supplied hardware labels or benchmark correctness.",
            "No performance budget, comparative parity, clean-machine compatibility, or independent acceptance is established.",
        ],
    }


# Public names kept explicit for tests and future callers.
build_benchmark_manifest = build_manifest
verify_benchmark_manifest = verify_manifest


def main(argv: list[str] | None = None) -> int:
    p = argparse.ArgumentParser(); sub = p.add_subparsers(dest="command", required=True)
    c = sub.add_parser("create"); c.add_argument("spec", type=Path); c.add_argument("package_root", type=Path); c.add_argument("release_manifest", type=Path); c.add_argument("evidence_root", type=Path); c.add_argument("output", type=Path)
    v = sub.add_parser("verify"); v.add_argument("manifest", type=Path); v.add_argument("package_root", type=Path); v.add_argument("release_manifest", type=Path); v.add_argument("evidence_root", type=Path); v.add_argument("--json", type=Path)
    args = p.parse_args(argv)
    try:
        if args.command == "create":
            result = build_manifest(_load_json(args.spec, MAX_SPEC_BYTES, "benchmark spec"), args.package_root, args.release_manifest, args.evidence_root)
            args.output.parent.mkdir(parents=True, exist_ok=True)
            args.output.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        else:
            result = verify_manifest(_load_json(args.manifest, MAX_MANIFEST_BYTES, "benchmark manifest"), args.package_root, args.release_manifest, args.evidence_root)
            if args.json:
                args.json.parent.mkdir(parents=True, exist_ok=True)
                args.json.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    except (BenchmarkManifestError, release_manifest.ManifestError, OSError) as exc:
        print(f"Benchmark manifest {args.command} failed: {exc}", file=sys.stderr); return 1
    print(json.dumps(result, indent=2, sort_keys=True)); return 0


if __name__ == "__main__":
    raise SystemExit(main())
