#!/usr/bin/env python3
"""Stage an already-tested Release build, its content, and app-local VC runtime."""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import shutil
import subprocess

from inspect_pe_dependencies import inspect_file

ROOT = Path(__file__).resolve().parents[1]
EXECUTABLES = (
    "AstralGame", "AstralEditor", "AstralPlayerWin32", "AstralPlayer",
    "AstralEnginePlayerWin32", "AstralEnginePlayer", "AstralProject", "AstralCapture",
)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, required=True)
    parser.add_argument("--crt-dir", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    if output.exists():
        parser.error("Output must be a new directory; existing packages are preserved.")
    build = args.build_dir.resolve() / "Release"
    cache = (args.build_dir / "CMakeCache.txt").read_text(encoding="utf-8")
    source_line = next(line for line in cache.splitlines() if line.startswith("CMAKE_HOME_DIRECTORY:INTERNAL="))
    if Path(source_line.split("=", 1)[1]).resolve() != ROOT:
        raise ValueError("Build directory belongs to a different source checkout.")
    # Validate all inputs and the entire imported VC runtime closure before writing.
    inputs = {name + ".exe": build / (name + ".exe") for name in EXECUTABLES}
    reports = {}
    pending = list(inputs)
    while pending:
        name = pending.pop()
        report = inspect_file(inputs[name])
        if report["machine"] != "AMD64" or report["analysis"]["debug_crt_imports"]:
            raise ValueError(f"Not a distributable x64 Release binary: {name}")
        reports[name] = report["all_imports"]
        for dll in report["analysis"]["vc_runtime_imports"]:
            if dll not in inputs:
                inputs[dll] = args.crt_dir / dll
                pending.append(dll)
    source_commit = subprocess.check_output(
        ["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip()
    dirty = subprocess.check_output(
        ["git", "status", "--porcelain", "--untracked-files=normal"], cwd=ROOT, text=True).strip()
    if dirty:
        raise ValueError("Commit reviewed source before packaging so the manifest identifies exact source.")
    output.mkdir(parents=True)
    for name, source in inputs.items():
        shutil.copy2(source, output / name)
    shutil.copytree(ROOT / "Content" / "Samples" / "Playground", output / "Content")
    shutil.copytree(ROOT / "Tests" / "Fixtures" / "NativeGpu", output / "GpuContent")
    shutil.copy2(ROOT / "Docs" / "PLAYTEST.md", output / "START-HERE.md")
    for category in ("QA", "Reviews"):
        report_path = Path("Docs") / category / "PLAYTEST-WINDOWS-2026-10-29.md"
        (output / report_path.parent).mkdir(parents=True)
        shutil.copy2(ROOT / report_path, output / report_path)
    launchers = {
        "Play AnimeRPG.cmd": ('set "ASTRAL_RENDER_MODE=astral"\n', 'AstralGame.exe'),
        "Play Engine Sample.cmd": ("", 'AstralPlayerWin32.exe "Content" "project.json"'),
        "Open Astral Editor.cmd": ("", 'AstralEditor.exe "Content" "project.json"'),
        "Open Engine Player.cmd": ("", 'AstralEnginePlayerWin32.exe "GpuContent" "project.json"'),
        "Open GPU Sample.cmd": ("", 'AstralEnginePlayerWin32.exe "GpuContent" "project.json"'),
    }
    for name, (environment, command) in launchers.items():
        # pushd supports paths with spaces and UNC locations; environment is scoped.
        text = '@echo off\nsetlocal\npushd "%~dp0"\n' + environment
        text += 'start "" /wait ' + command + '\nset "result=%errorlevel%"\npopd\nexit /b %result%\n'
        (output / name).write_text(text, encoding="utf-8", newline="\r\n")
    files = {
        path.relative_to(output).as_posix(): hashlib.sha256(path.read_bytes()).hexdigest()
        for path in sorted(output.rglob("*")) if path.is_file()
    }
    manifest = {
        "source_commit": source_commit, "configuration": "Release", "platform": "Windows x64",
        "build_directory": str(args.build_dir.resolve()), "source_directory": str(ROOT),
        "created_utc": datetime.now(timezone.utc).isoformat(),
        "files_sha256": files, "binary_imports": reports,
        "scope": "Local combat prototype and engine tools; not full RPG or long-soak acceptance.",
    }
    (output / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    print(f"Packaged {len(files)} files from {source_commit} into {output}")


if __name__ == "__main__":
    main()
