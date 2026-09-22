#!/usr/bin/env python3
"""Exercise the CMake test contract with a real compiler, not the Win32 runtime.

The tiny fixtures deliberately have no engine dependencies. PASS is evidence for
build/test configuration only; it is not a native game or interactive smoke pass.
"""
from __future__ import annotations

import json
import re
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


def run(*args: str, expected_success: bool = True) -> subprocess.CompletedProcess[str]:
    result = subprocess.run(args, text=True, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, timeout=180, check=False)
    if expected_success and result.returncode:
        raise AssertionError(f"Command failed: {args!r}\n{result.stdout}")
    return result


class TestSafety(unittest.TestCase):
    def test_all_native_targets_use_the_safety_helper(self) -> None:
        cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
        targets = set(re.findall(r"add_executable\((\w+)", cmake)) - {"AstralGame"}
        guarded = set(re.findall(r"astral_add_test\((\w+)", cmake))
        self.assertEqual(targets, guarded)
        self.assertTrue(guarded)
        self.assertNotRegex(cmake, r"(?m)^\s*add_test\(")

    def test_release_evaluates_assertions_and_serializes_smokes(self) -> None:
        with tempfile.TemporaryDirectory(prefix="astral-test-contract-") as directory:
            source = Path(directory) / "source"
            build = Path(directory) / "build"
            source.mkdir()
            # No abort dialog: disabled assertions instead produce a normal failure.
            (source / "test.cpp").write_text(
                '#include <cassert>\nint main() { int seen = 0; '
                'assert(++seen == 1); return seen == 1 ? 0 : 9; }\n', encoding="utf-8")
            (source / "game.cpp").write_text(
                '#ifndef NDEBUG\n#error Product Release flags were changed\n#endif\n'
                'int main() { return 0; }\n', encoding="utf-8")
            module = (ROOT / "cmake/AstralTestSafety.cmake").as_posix()
            (source / "CMakeLists.txt").write_text(
                'cmake_minimum_required(VERSION 3.25)\nproject(SafetyFixture LANGUAGES CXX)\n'
                'enable_testing()\n'
                f'include("{module}")\n'
                'add_executable(AstralGame game.cpp)\n'
                'add_executable(DomainTest test.cpp)\nastral_add_test(DomainTest)\n'
                'add_executable(FixtureRuntimeSmoke test.cpp)\n'
                'astral_add_test(FixtureRuntimeSmoke)\n', encoding="utf-8")
            run("cmake", "-S", str(source), "-B", str(build), "-DCMAKE_BUILD_TYPE=Release")
            run("cmake", "--build", str(build), "--config", "Release")
            run("ctest", "--test-dir", str(build), "-C", "Release", "--output-on-failure")
            info = json.loads(run("ctest", "--test-dir", str(build), "-C", "Release",
                                  "--show-only=json-v1").stdout)
            tests = {test["name"]: {p["name"]: p["value"] for p in test["properties"]}
                     for test in info["tests"]}
            self.assertTrue(tests["FixtureRuntimeSmoke"]["RUN_SERIAL"])
            self.assertEqual(tests["FixtureRuntimeSmoke"]["TIMEOUT"], 180)
            self.assertEqual(tests["DomainTest"]["TIMEOUT"], 60)
            self.assertFalse(tests["DomainTest"].get("RUN_SERIAL", False))

    def test_missing_assertion_flag_fails_compilation(self) -> None:
        with tempfile.TemporaryDirectory(prefix="astral-assert-negative-") as directory:
            source = Path(directory) / "source"
            build = Path(directory) / "build"
            source.mkdir()
            guard = (ROOT / "Tests/AssertionsEnabled.cpp").as_posix()
            (source / "CMakeLists.txt").write_text(
                'cmake_minimum_required(VERSION 3.25)\nproject(NegativeFixture LANGUAGES CXX)\n'
                f'add_library(DisabledAssertions OBJECT "{guard}")\n'
                'target_compile_definitions(DisabledAssertions PRIVATE NDEBUG)\n',
                encoding="utf-8")
            run("cmake", "-S", str(source), "-B", str(build))
            result = run("cmake", "--build", str(build), "--config", "Release",
                         expected_success=False)
            self.assertNotEqual(result.returncode, 0, result.stdout)
            self.assertIn("Astral test assertions are disabled", result.stdout)


if __name__ == "__main__":
    unittest.main(verbosity=2)
