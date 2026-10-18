#!/usr/bin/env python3
"""Build Release engine tools and stage an intact project without R0/game runners."""
from __future__ import annotations

import argparse
from dataclasses import dataclass
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import re
import signal
import stat
import subprocess
import sys
import time

sys.dont_write_bytecode = True
import release_manifest

TARGETS = ("AstralEnginePlayerWin32", "AstralEnginePlayer", "AstralProject", "AstralEditor")
MAX_FILES = 100_000
MAX_BYTES = 2 * 1024**3
SYSTEM_DLLS = frozenset(name.casefold() for name in (
    "KERNEL32.dll", "USER32.dll", "GDI32.dll", "WINMM.dll", "SHELL32.dll",
    "ADVAPI32.dll", "COMDLG32.dll", "OLE32.dll", "OLEAUT32.dll", "COMCTL32.dll",
    "IMM32.dll", "VERSION.dll", "SHLWAPI.dll", "NTDLL.dll", "WS2_32.dll"))


class PackageError(ValueError):
    pass


def utc_now():
    return datetime.now(timezone.utc).isoformat()


def digest(path):
    hasher = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            hasher.update(chunk)
    return hasher.hexdigest()


def under(path, root):
    return path == root or root in path.parents


def overlaps(left, right):
    return under(left, right) or under(right, left)


def ordinary(path, *, directory=False):
    info = path.lstat()
    if stat.S_ISLNK(info.st_mode) or getattr(info, "st_file_attributes", 0) & 0x400:
        raise PackageError(f"linked/reparse entry is not a package input: {path}")
    if directory:
        if not stat.S_ISDIR(info.st_mode):
            raise PackageError(f"expected a directory: {path}")
    elif not stat.S_ISREG(info.st_mode) or info.st_nlink != 1:
        raise PackageError(f"expected an ordinary single-link file: {path}")
    return info


def relative_name(name):
    if not isinstance(name, str) or not name or "\\" in name or ":" in name or "\0" in name:
        raise PackageError(f"invalid relative path: {name!r}")
    path = PurePosixPath(name)
    if not path.parts or path.is_absolute() or path.as_posix() != name or any(p in (".", "..") for p in path.parts):
        raise PackageError(f"invalid relative path: {name!r}")
    for part in path.parts:
        stem = part.split(".")[0].casefold()
        if part.endswith((" ", ".")) or any(c in part for c in '<>"|?*') or any(ord(c) < 32 for c in part):
            raise PackageError(f"invalid Windows filename: {name!r}")
        if stem in {"con", "prn", "aux", "nul", *(f"com{i}" for i in "123456789\u00b9\u00b2\u00b3"), *(f"lpt{i}" for i in "123456789\u00b9\u00b2\u00b3")}:
            raise PackageError(f"reserved Windows filename: {name!r}")
    return path


@dataclass(frozen=True)
class InputFile:
    source: Path
    relative: str
    size: int
    sha256: str
    identity: tuple
    directory: bool = False


def identity(info):
    return info.st_dev, info.st_ino, info.st_mtime_ns, info.st_mode, info.st_size


def input_file(path, relative):
    before = ordinary(path)
    checksum = digest(path)
    if identity(ordinary(path)) != identity(before):
        raise PackageError(f"input changed during preflight: {path}")
    return InputFile(path, relative, before.st_size, checksum, identity(before))


def inventory(content, *, max_files=MAX_FILES, max_bytes=MAX_BYTES):
    """Read-only preflight; do not follow any internal directory aliases."""
    ordinary(content, directory=True)
    result, seen, total = [], set(), 0
    pending = [content]
    while pending:
        current = pending.pop()
        ordinary(current, directory=True)
        for entry in sorted(current.iterdir(), key=lambda p: p.name):
            relative = entry.relative_to(content).as_posix()
            relative_name(relative)
            folded = relative.casefold()
            if folded in seen:
                raise PackageError(f"Windows-case path collision: {relative}")
            seen.add(folded)
            info = entry.lstat()
            if stat.S_ISDIR(info.st_mode):
                info = ordinary(entry, directory=True)
                result.append(InputFile(entry, relative, 0, "", identity(info), True))
                if len(result) > max_files:
                    raise PackageError("Content exceeds the declared entry budget")
                pending.append(entry)
                continue
            info = ordinary(entry)
            total += info.st_size
            if len(result) >= max_files or total > max_bytes:
                raise PackageError("Content exceeds the declared file/byte budget")
            result.append(input_file(entry, relative))
    return sorted(result, key=lambda item: item.relative)


def copy_checked(item, target):
    if identity(ordinary(item.source, directory=item.directory)) != item.identity:
        raise PackageError(f"input identity changed after preflight: {item.source}")
    if item.directory:
        target.mkdir(parents=True, exist_ok=False)
        return
    if item.source.stat().st_size != item.size or digest(item.source) != item.sha256:
        raise PackageError(f"input changed after preflight: {item.source}")
    target.parent.mkdir(parents=True, exist_ok=True)
    hasher = hashlib.sha256()
    copied = 0
    with item.source.open("rb") as source, target.open("xb") as output:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            copied += len(chunk)
            if copied > item.size:
                raise PackageError(f"input grew during copy: {item.source}")
            hasher.update(chunk)
            output.write(chunk)
    if identity(ordinary(item.source)) != item.identity:
        raise PackageError(f"input identity changed while staging: {item.source}")
    if copied != item.size or hasher.hexdigest() != item.sha256 or digest(target) != item.sha256:
        raise PackageError(f"input/copy changed while staging: {item.source}")
    if digest(item.source) != item.sha256:
        raise PackageError(f"input changed while staging: {item.source}")


def verify_content_snapshot(content, files):
    if inventory(content) != files:
        raise PackageError("Content changed during staging; partial package retained")


def inspect_imports(text):
    if not re.search(r"^\s*8664 machine \(x64\)\s*$", text, re.MULTILINE | re.IGNORECASE):
        raise PackageError("dumpbin did not identify an x64 PE image")
    lines = text.splitlines()
    markers = [i for i, line in enumerate(lines) if line.strip() == "Image has the following dependencies:"]
    if len(markers) != 1:
        raise PackageError("dumpbin import section was not recognized")
    imports = []
    ended = False
    for line in lines[markers[0]+1:]:
        entry = line.strip()
        if not entry:
            continue
        if entry == "Summary":
            ended = True
            break
        if not re.fullmatch(r"[A-Za-z0-9_.-]+\.dll", entry, re.IGNORECASE):
            raise PackageError(f"unrecognized dumpbin dependency entry: {entry!r}")
        if entry.casefold() not in SYSTEM_DLLS:
            raise PackageError(f"unbundled/non-system PE dependency: {entry}")
        imports.append(entry)
    if not ended:
        raise PackageError("dumpbin dependency section has no recognized Summary terminator")
    imports = sorted(set(imports), key=str.casefold)
    if not imports:
        raise PackageError("dumpbin reported no DLL dependencies; cannot establish PE inventory")
    return imports


def source_binding(source):
    def git(*args):
        return subprocess.check_output(["git", "-C", str(source), *args], text=True, encoding="utf-8").strip()
    if git("status", "--porcelain", "--untracked-files=all"):
        raise PackageError("source checkout must be clean, including untracked files")
    return {"commit": git("rev-parse", "HEAD"), "tree": git("rev-parse", "HEAD^{tree}")}


class Runner:
    def __init__(self, evidence):
        self.evidence = evidence
        self.steps = []

    def run(self, label, argv, *, cwd, timeout=1200):
        started, tick = utc_now(), time.monotonic()
        env = os.environ.copy()
        env["MSBUILDDISABLENODEREUSE"] = "1"
        env["CMAKE_BUILD_PARALLEL_LEVEL"] = "2"
        log = self.evidence / (label + ".log")
        timed_out = False
        with log.open("xb") as stream:
            options = {"creationflags": subprocess.CREATE_NO_WINDOW} if os.name == "nt" else {"start_new_session": True}
            process = subprocess.Popen([str(a) for a in argv], cwd=cwd, env=env, stdout=stream,
                                       stderr=subprocess.STDOUT, **options)
            try:
                process.wait(timeout=timeout)
            except subprocess.TimeoutExpired:
                timed_out = True
                terminate_owned_tree(process)
            except BaseException:
                terminate_owned_tree(process)
                raise
        self.steps.append({"label": label, "argv": [str(a) for a in argv], "cwd": str(cwd),
                           "started": started, "ended": utc_now(), "seconds": time.monotonic()-tick,
                           "pid": process.pid, "exit": process.returncode, "timed_out": timed_out, "log": str(log)})
        (self.evidence / "steps.json").write_text(json.dumps(self.steps, indent=2)+"\n", encoding="utf-8")
        if process.returncode or timed_out:
            raise PackageError(f"{label} failed with exit {process.returncode}; see {log}")
        return log.read_text(encoding="utf-8", errors="replace")


def terminate_owned_tree(process):
    """Only the still-live Popen child and its descendants belong to this call."""
    if process.poll() is not None:
        return
    if os.name == "nt":
        subprocess.run(["taskkill.exe", "/PID", str(process.pid), "/T", "/F"],
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                       timeout=15, check=False, creationflags=subprocess.CREATE_NO_WINDOW)
    else:
        try:
            os.killpg(process.pid, signal.SIGKILL)
        except ProcessLookupError:
            pass
    if process.poll() is None:
        process.kill()
    process.wait(timeout=15)


def admit_build(evidence, phase):
    if os.name != "nt":
        raise PackageError("this Release x64 package command requires Windows and its installed MSVC tools")
    available = int(subprocess.check_output(["powershell.exe", "-NoProfile", "-NonInteractive", "-Command",
        "(Get-CimInstance Win32_OperatingSystem).FreePhysicalMemory"], text=True, timeout=30,
        creationflags=subprocess.CREATE_NO_WINDOW).strip()) * 1024
    (evidence / (phase + "-memory.json")).write_text(json.dumps({"utc": utc_now(), "available_bytes": available})+"\n", encoding="utf-8")
    if available < 8 * 1024**3:
        raise PackageError("less than 8 GiB available before the bounded build")


def stage(args):
    source = args.source.resolve(strict=True)
    content = args.content.resolve(strict=True)
    parent = args.destination.parent.resolve(strict=True)
    ordinary(parent, directory=True)
    relative_name(args.destination.name)
    destination = parent / args.destination.name
    if os.path.lexists(destination):
        raise PackageError("package destination already exists; nothing will be overwritten")
    build = args.build.resolve(strict=False)
    evidence = args.evidence.resolve(strict=False)
    for name, path in (("source", source), ("build", build), ("content", content), ("evidence", evidence)):
        if overlaps(destination, path):
            raise PackageError(f"package destination overlaps {name}")
    if overlaps(build, source) or overlaps(build, content) or overlaps(evidence, source) or overlaps(evidence, content) or overlaps(evidence, build):
        raise PackageError("build/evidence directories must be external and separate")
    original = source_binding(source)
    files = inventory(content)
    if not any(item.relative == "project.json" and not item.directory for item in files):
        raise PackageError("Content/project.json is required for the native player's packaged default")
    notices = []
    for notice in args.notice:
        notice = notice.resolve(strict=True)
        relative_name(notice.name)
        notices.append(input_file(notice, notice.name))
    for name in ("LICENSE", "LICENSE.txt", "LICENSE.md", "NOTICE", "NOTICE.txt", "THIRD_PARTY_NOTICES.txt"):
        notice = source / name
        if os.path.lexists(notice):
            notices.append(input_file(notice, name))
    if len({item.relative.casefold() for item in notices}) != len(notices):
        raise PackageError("notice filenames collide")
    cache = build / "CMakeCache.txt"
    if cache.exists():
        values = dict(line.split("=", 1) for line in cache.read_text(encoding="utf-8").splitlines() if "=" in line and not line.startswith(("#", "//")))
        home = values.get("CMAKE_HOME_DIRECTORY:INTERNAL")
        if home is None or Path(home).resolve(strict=True) != source:
            raise PackageError("existing build cache belongs to another source")
    elif build.exists() and any(build.iterdir()):
        raise PackageError("nonempty build directory has no attributable CMake cache")
    cmake = args.cmake.resolve(strict=True)
    dumpbin = args.dumpbin.resolve(strict=True)
    ordinary(cmake)
    ordinary(dumpbin)
    evidence.mkdir(parents=True, exist_ok=False)
    runner = Runner(evidence)
    admit_build(evidence, "configure")
    runner.run("configure", [cmake, "-S", source, "-B", build, "-G", "Visual Studio 17 2022", "-A", "x64",
                             "-DBUILD_TESTING=OFF", "-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded"], cwd=source)
    admit_build(evidence, "build")
    runner.run("build-release", [cmake, "--build", build, "--config", "Release", "--target", *TARGETS,
                                 "--parallel", str(args.jobs)], cwd=source)
    executables, pe = [], {}
    for target in TARGETS:
        executable = build / "Release" / (target + ".exe")
        info = ordinary(executable)
        headers = runner.run("headers-"+target, [dumpbin, "/headers", executable], cwd=source, timeout=30)
        dependents = runner.run("imports-"+target, [dumpbin, "/dependents", executable], cwd=source, timeout=30)
        pe[target] = inspect_imports(headers + "\n" + dependents)
        executables.append(input_file(executable, executable.name))
    if source_binding(source) != original:
        raise PackageError("source changed during build; no package reserved")
    if len(notices) > 64 or sum(item.size for item in [*files, *notices, *executables]) > MAX_BYTES:
        raise PackageError("complete package input exceeds the declared count/byte budget")
    destination.mkdir()  # exclusive reservation; never replace or recursively remove anything
    for item in executables:
        copy_checked(item, destination / item.relative)
    for item in files:
        copy_checked(item, destination / "Content" / item.relative)
    for item in notices:
        copy_checked(item, destination / "Notices" / item.relative)
    (destination / "START-HERE.txt").write_text(
        "Astral Engine project\n\nDouble-click AstralEnginePlayerWin32.exe to play Content/project.json.\n"
        "To edit: AstralEditor.exe <absolute path to Content> project.json\n"
        "To create another project: AstralProject.exe create <existing parent> <new directory> --template 3d --name <display name>\n"
        "Headless validation: AstralEnginePlayer.exe <absolute path to Content> project.json --frames 120 --capture <output folder>\n"
        "This player uses the CPU renderer. BuildInfo.json records the source and PE dependencies.\n", encoding="utf-8")
    (destination / "BuildInfo.json").write_text(json.dumps({**original, "configuration": "Release", "architecture": "x64",
        "runtime_request": "MultiThreaded", "pe_imports": pe, "content_files": sum(not f.directory for f in files),
        "content_directories": sum(f.directory for f in files), "packaged_at": utc_now(),
        "acceptance": {"native_launch": False, "clean_machine": False, "gpu_presentation": False, "soak_24h": False}}, indent=2)+"\n", encoding="utf-8")
    if source_binding(source) != original:
        raise PackageError("source changed during staging; partial package retained")
    verify_content_snapshot(content, files)
    manifest = destination / "release-manifest.json"
    result = release_manifest.write_release_manifest(destination, manifest, original["commit"])
    verified = release_manifest.verify_manifest(result, destination, manifest_path=manifest,
        expected_commit=original["commit"], executable_path="AstralEnginePlayerWin32.exe",
        expected_executable_sha256=executables[0].sha256)
    (evidence / "package-result.json").write_text(json.dumps(verified, indent=2)+"\n", encoding="utf-8")
    return destination


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("source", "build", "content", "destination", "evidence", "cmake", "dumpbin"):
        parser.add_argument("--"+name, type=Path, required=True)
    parser.add_argument("--jobs", type=int, choices=(1, 2), default=2)
    parser.add_argument("--notice", type=Path, action="append", default=[])
    args = parser.parse_args(argv)
    try:
        output = stage(args)
    except (PackageError, release_manifest.ManifestError, OSError, subprocess.SubprocessError) as error:
        print(f"Engine packaging failed: {error}", file=sys.stderr)
        return 1
    print(f"Staged and byte-verified: {output}. Native launch remains a separate gate.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
