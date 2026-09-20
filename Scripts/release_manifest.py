#!/usr/bin/env python3
from __future__ import annotations
import argparse, hashlib, json, os, re, sys
from datetime import datetime
from pathlib import Path, PurePosixPath
from typing import Any

SCHEMA_VERSION = 2
MAX_MANIFEST_BYTES = 16 * 1024 * 1024
MAX_FILES = 100_000
SHA256_RE = re.compile(r"^[0-9a-fA-F]{64}$")
COMMIT_RE = re.compile(r"^[0-9a-fA-F]{40}$")

class ManifestError(ValueError): pass

def _iso_now() -> str:
    return datetime.now().astimezone().isoformat(timespec="seconds")

def _sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024*1024), b""):
            h.update(chunk)
    return h.hexdigest()

def _safe_rel(value: Any) -> str:
    if not isinstance(value, str) or not value or "\x00" in value or "\\" in value:
        raise ManifestError("manifest paths must be non-empty forward-slash relative paths")
    if ":" in value.split("/", 1)[0]:
        raise ManifestError(f"drive-qualified path: {value!r}")
    p = PurePosixPath(value)
    if p.is_absolute() or any(part in ("", ".", "..") for part in p.parts) or p.as_posix() != value:
        raise ManifestError(f"unsafe manifest path: {value!r}")
    return value

def _under(root: Path, child: Path) -> bool:
    try: return os.path.commonpath((str(root), str(child))) == str(root)
    except ValueError: return False

def _package_files(root: Path, manifest_path: Path | None = None):
    root = root.resolve(strict=True)
    skip = manifest_path.resolve(strict=False) if manifest_path else None
    count = 0
    def on_scan_error(exc: OSError) -> None:
        raise ManifestError(f"cannot scan package tree: {exc}") from exc

    for cur, dirs, files in os.walk(root, topdown=True, onerror=on_scan_error, followlinks=False):
        curp = Path(cur)
        dirs[:] = sorted(dirs)
        for d in list(dirs):
            p = curp / d
            if p.is_symlink() or not _under(root, p.resolve(strict=True)):
                raise ManifestError(f"unsafe package directory: {p}")
        for name in sorted(files):
            p = curp / name
            if skip is not None and p.resolve(strict=False) == skip: continue
            if p.is_symlink(): raise ManifestError(f"symlinked package file: {p}")
            rp = p.resolve(strict=True)
            if not _under(root, rp) or not rp.is_file(): raise ManifestError(f"unsafe package file: {p}")
            count += 1
            if count > MAX_FILES: raise ManifestError("package file-count limit exceeded")
            yield p

def build_manifest(package_root: Path, commit: str, *, run_started_at: str | None = None, manifest_path: Path | None = None) -> dict[str, Any]:
    if not COMMIT_RE.fullmatch(commit): raise ManifestError("commit must be 40 hex characters")
    root = package_root.resolve(strict=True)
    if not root.is_dir(): raise ManifestError("package root must be a directory")
    files, seen = [], set()
    for p in _package_files(root, manifest_path):
        rel = _safe_rel(p.relative_to(root).as_posix())
        key = rel.casefold()
        if key in seen: raise ManifestError(f"Windows-case path collision: {rel}")
        seen.add(key)
        files.append({"path": rel, "bytes": p.stat().st_size, "sha256": _sha256(p)})
    canonical = json.dumps(files, sort_keys=True, separators=(",", ":")).encode()
    return {"schema_version": SCHEMA_VERSION, "generated_at": _iso_now(), "run_started_at": run_started_at,
            "commit": commit.lower(), "package_root": str(root), "files": files,
            "entries_sha256": hashlib.sha256(canonical).hexdigest()}

def write_release_manifest(package_root: Path, manifest_path: Path, commit: str, *, run_started_at: str | None = None) -> dict[str, Any]:
    result = build_manifest(package_root, commit, run_started_at=run_started_at, manifest_path=manifest_path)
    manifest_path.parent.mkdir(parents=True, exist_ok=True)
    manifest_path.write_text(json.dumps(result, indent=2, sort_keys=True)+"\n", encoding="utf-8")
    return result

def load_manifest(path: Path) -> dict[str, Any]:
    try:
        if path.stat().st_size > MAX_MANIFEST_BYTES: raise ManifestError("manifest byte limit exceeded")
        data = json.loads(path.read_text(encoding="utf-8"))
    except ManifestError: raise
    except Exception as e: raise ManifestError(f"cannot read manifest: {e}") from e
    if not isinstance(data, dict): raise ManifestError("manifest root must be an object")
    return data

def verify_manifest(manifest: dict[str, Any], package_root: Path, *, manifest_path: Path | None = None,
                    expected_commit: str | None = None, expected_executable_sha256: str | None = None,
                    executable_path: str = "AstralGame.exe") -> dict[str, Any]:
    schema = manifest.get("schema_version", 1)
    if schema not in (1, SCHEMA_VERSION): raise ManifestError(f"unsupported schema_version {schema!r}")
    commit = manifest.get("commit")
    if not isinstance(commit, str) or not COMMIT_RE.fullmatch(commit): raise ManifestError("invalid manifest commit")
    if expected_commit is not None and commit.lower() != expected_commit.lower(): raise ManifestError("commit mismatch")
    items = manifest.get("files")
    if not isinstance(items, list) or len(items) > MAX_FILES: raise ManifestError("invalid files array")
    expected, seen = {}, set()
    ordered = []
    for item in items:
        if not isinstance(item, dict): raise ManifestError("file entry must be an object")
        rel = _safe_rel(item.get("path")); key = rel.casefold()
        if key in seen: raise ManifestError(f"duplicate/colliding path: {rel}")
        seen.add(key)
        size, digest = item.get("bytes"), item.get("sha256")
        if not isinstance(size, int) or isinstance(size, bool) or size < 0: raise ManifestError(f"invalid size for {rel}")
        if not isinstance(digest, str) or not SHA256_RE.fullmatch(digest): raise ManifestError(f"invalid hash for {rel}")
        expected[rel] = {"bytes": size, "sha256": digest.lower()}; ordered.append({"path": rel, "bytes": size, "sha256": digest.lower()})
    if schema == SCHEMA_VERSION:
        agg = manifest.get("entries_sha256")
        if not isinstance(agg, str) or not SHA256_RE.fullmatch(agg): raise ManifestError("missing entries_sha256")
        canonical = json.dumps(ordered, sort_keys=True, separators=(",", ":")).encode()
        if hashlib.sha256(canonical).hexdigest() != agg.lower(): raise ManifestError("entries_sha256 mismatch")
    root = package_root.resolve(strict=True)
    actual, actual_cf = {}, set()
    for p in _package_files(root, manifest_path):
        rel = _safe_rel(p.relative_to(root).as_posix()); key = rel.casefold()
        if key in actual_cf: raise ManifestError(f"Windows-case package collision: {rel}")
        actual_cf.add(key); actual[rel]=p
    missing, extra = sorted(set(expected)-set(actual)), sorted(set(actual)-set(expected))
    if missing: raise ManifestError("missing files: "+", ".join(missing))
    if extra: raise ManifestError("unmanifested files: "+", ".join(extra))
    verified=[]
    for rel in sorted(expected):
        p=actual[rel]
        if p.stat().st_size != expected[rel]["bytes"]: raise ManifestError(f"size mismatch for {rel}")
        digest=_sha256(p)
        if digest != expected[rel]["sha256"]: raise ManifestError(f"SHA-256 mismatch for {rel}")
        verified.append({"path":rel,"bytes":p.stat().st_size,"sha256":digest})
    exe_rel=_safe_rel(executable_path)
    if expected_executable_sha256 is not None:
        if not SHA256_RE.fullmatch(expected_executable_sha256): raise ManifestError("malformed expected executable hash")
        if exe_rel not in expected or expected[exe_rel]["sha256"] != expected_executable_sha256.lower(): raise ManifestError("executable hash mismatch")
    return {"schema_version":1,"manifest_schema_version":schema,"package_root_verified":str(root),
            "recorded_package_root":manifest.get("package_root"),"commit":commit.lower(),"files_verified":len(verified),
            "verified_files":verified,"manifest_integrity_verified":True,
            "acceptance":{"package_manifest_verified":True,"package_launch_verified":False,
                          "clean_machine_compatibility_verified":False,"independent_acceptance":False},
            "limitations":["Verification covers bytes under the explicit package root only.",
                           "The recorded package_root is informational and is not trusted.",
                           "This tool does not launch the package or prove Visual C++ prerequisite installation.",
                           "This tool does not establish clean-machine compatibility, code-signing trust, or independent acceptance."]}

def parse_args(argv=None):
    p=argparse.ArgumentParser(); sub=p.add_subparsers(dest="command", required=True)
    c=sub.add_parser("create"); c.add_argument("package_root", type=Path); c.add_argument("manifest", type=Path); c.add_argument("--commit", required=True); c.add_argument("--run-started-at")
    v=sub.add_parser("verify"); v.add_argument("manifest", type=Path); v.add_argument("package_root", type=Path); v.add_argument("--expected-commit"); v.add_argument("--expected-executable-sha256"); v.add_argument("--executable-path", default="AstralGame.exe"); v.add_argument("--json", type=Path)
    return p.parse_args(argv)

def main(argv=None):
    args=parse_args(argv)
    try:
        if args.command=="create": result=write_release_manifest(args.package_root,args.manifest,args.commit,run_started_at=args.run_started_at)
        else: result=verify_manifest(load_manifest(args.manifest),args.package_root,manifest_path=args.manifest,expected_commit=args.expected_commit,expected_executable_sha256=args.expected_executable_sha256,executable_path=args.executable_path)
    except (ManifestError,OSError) as e:
        print(f"Release manifest {args.command} failed: {e}", file=sys.stderr); return 1
    text=json.dumps(result,indent=2,sort_keys=True)+"\n"; print(text,end="")
    if args.command=="verify" and args.json is not None:
        args.json.parent.mkdir(parents=True,exist_ok=True); args.json.write_text(text,encoding="utf-8")
    return 0
if __name__=="__main__": raise SystemExit(main())
