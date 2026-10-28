#!/usr/bin/env python3
"""Offline portable-build inventory and CI path-filter contracts; no compiler."""
from __future__ import annotations

import fnmatch
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]
WORKFLOW = ROOT / ".github/workflows/engine-runtime.yml"
PORTABLE = ROOT / "Tests/EngineRuntime/CMakeLists.txt"
SOURCES = ROOT / "cmake/AstralEngineSources.cmake"


def event_paths(text: str, event: str) -> list[str]:
    """Read this workflow's block-style literal filters, rejecting unknown forms.

    This deliberately is not a general YAML parser. A structural workflow change
    must update this small source contract rather than silently skip coverage.
    """
    event_block = re.search(
        rf"(?ms)^  {re.escape(event)}:\n(.*?)(?=^  [A-Za-z_]+:|^\S|\Z)", text
    )
    if event_block is None:
        raise AssertionError(f"missing event block: {event}")
    paths_block = re.search(
        r"(?ms)^    paths:\n(.*?)(?=^    [A-Za-z_]+:|\Z)", event_block.group(1)
    )
    if paths_block is None:
        raise AssertionError(f"missing paths block: {event}")
    paths = []
    for line in paths_block.group(1).splitlines():
        if not line.strip() or line.lstrip().startswith("#"):
            continue
        match = re.fullmatch(r'      - "([^"\n]+)"', line)
        if match is None:
            raise AssertionError(f"unsupported {event} path-filter syntax: {line!r}")
        paths.append(match.group(1))
    if not paths:
        raise AssertionError(f"empty paths block: {event}")
    return paths


def path_matches(path: str, pattern: str) -> bool:
    # Existing filters use literal segments, segment '*' and trailing '/**'.
    # Unlike fnmatch on a whole string, GitHub's single '*' does not span '/'.
    if pattern.endswith("/**"):
        return path.startswith(pattern[:-2])
    path_parts, pattern_parts = path.split("/"), pattern.split("/")
    return len(path_parts) == len(pattern_parts) and all(
        fnmatch.fnmatchcase(part, glob) for part, glob in zip(path_parts, pattern_parts)
    )


class PortableBuildContractTests(unittest.TestCase):
    def assert_ci_covers(self, paths: set[str]) -> None:
        workflow = WORKFLOW.read_text(encoding="utf-8")
        for event in ("pull_request", "push"):
            patterns = event_paths(workflow, event)
            for path in sorted(paths):
                with self.subTest(event=event, path=path):
                    self.assertTrue(
                        any(path_matches(path, pattern) for pattern in patterns),
                        f"{event} portable CI does not trigger for build input {path}",
                    )

    def test_direct_portable_cmake_and_tool_inputs_trigger_ci(self) -> None:
        project = PORTABLE.read_text(encoding="utf-8")
        inputs = set(re.findall(r'"\$\{ASTRAL_ROOT\}/([^"\n]+\.(?:cmake|cpp))"', project))
        inputs = {path for path in inputs if "$" not in path}
        self.assertGreaterEqual(len(inputs), 6)
        self.assert_ci_covers(inputs | {PORTABLE.relative_to(ROOT).as_posix()})

    def test_assertion_guard_and_contract_script_trigger_ci(self) -> None:
        self.assert_ci_covers({
            "Tests/AssertionsEnabled.cpp",
            Path(__file__).relative_to(ROOT).as_posix(),
        })

    def test_portable_job_runs_offline_contract_before_configure(self) -> None:
        workflow = WORKFLOW.read_text(encoding="utf-8")
        contract = "run: python3 Scripts/test_portable_build_contract.py"
        self.assertIn(contract, workflow)
        self.assertLess(workflow.index(contract), workflow.index("      - name: Configure"))

    def test_source_catalog_and_declared_suite_files_exist(self) -> None:
        catalog = SOURCES.read_text(encoding="utf-8")
        sources = re.findall(r'"\$\{ASTRAL_ENGINE_ROOT\}/([^"\n]+)"', catalog)
        self.assertTrue(sources)
        for source in sources:
            with self.subTest(source=source):
                self.assertTrue((ROOT / source).is_file(), f"missing source: {source}")
        suite_block = re.search(r"set\(ASTRAL_ENGINE_TEST_SUITES\s+(.*?)\)", catalog, re.S)
        self.assertIsNotNone(suite_block)
        suites = suite_block.group(1).split()
        self.assertTrue(suites)
        self.assertEqual(len(suites), len(set(suites)), "duplicate portable test suites")
        for suite in suites:
            with self.subTest(suite=suite):
                self.assertTrue((ROOT / "Tests" / f"{suite}.cpp").is_file())


if __name__ == "__main__":
    unittest.main(verbosity=2)
