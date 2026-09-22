#!/usr/bin/env python3
"""Reproduce the frozen hourly-worker SETUP checks, not gameplay acceptance.

Run from any directory using a clean checkout of the setup candidate. The
baseline and head are explicit so later game development is not misclassified
as an unauthorized change to this historical setup packet. Uses Python's
standard library and Git only; performs reads, never checkout/reset/write.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import unittest

ROOT = Path(__file__).resolve().parents[3]
PATHS = (
    "AGENTS.md",
    "GAME_DEVELOPMENT_CONTROL.md",
    "Docs/Agents/ANIMERPG-HOURLY.md",
    "Docs/Agents/animerpg-hourly/BACKLOG.json",
    "Docs/Agents/animerpg-hourly/validate_setup.py",
    "Tasks/GAME-HOURLY-SETUP-2026-09-22.md",
)
FILES: dict[str, str] = {}
DATA: dict = {}
BASE = ""
HEAD = ""


def git(*args: str) -> bytes:
    """Bound every read-only Git subprocess and propagate failures."""
    return subprocess.run(
        ["git", "-C", str(ROOT), *args], check=True,
        stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=20,
    ).stdout


def blob_sha(data: bytes) -> str:
    return hashlib.sha1(b"blob " + str(len(data)).encode() + b"\0" + data).hexdigest()


class SetupChecks(unittest.TestCase):
    def test_exact_scope(self):
        changed = git("diff", "--name-only", "-z", f"{BASE}...{HEAD}").decode().split("\0")
        self.assertEqual(set(filter(None, changed)), set(PATHS))
        deleted = git("diff", "--name-only", "--diff-filter=D", "-z", f"{BASE}...{HEAD}")
        self.assertEqual(deleted, b"")

    def test_utf8_roundtrip_and_files(self):
        for path, text in FILES.items():
            with self.subTest(path=path):
                self.assertEqual((ROOT / path).read_bytes().replace(b"\r\n", b"\n"), text.encode("utf-8"))
                self.assertTrue(text.endswith("\n"))

    def test_existing_rules_preserved(self):
        original = FILES["AGENTS.md"].split("\n## September 22, 2026:")[0]
        self.assertEqual(blob_sha(original.encode()), "2945c7a09cc10cb4806d2ff9a7d79f78daabe16a")

    def test_engine_control_preserved(self):
        text = FILES["GAME_DEVELOPMENT_CONTROL.md"]
        original = text.split("\n## September 22, 2026 operator update:")[0]
        self.assertEqual(blob_sha(original.encode()), "90d21ad6fb8c231518ba2f3384b7a3b9509887ff")
        self.assertIn("not a task packet unpausing itself", text)
        self.assertIn("independent implementation-review", text)

    def test_unique_ids(self):
        for key in ("sources", "features"):
            ids = [item["id"] for item in DATA[key]]
            self.assertEqual(len(ids), len(set(ids)))

    def test_five_plus_one(self):
        self.assertEqual(len(DATA["features"]), 6)
        self.assertEqual(sum(f["kind"] == "reference_feature" for f in DATA["features"]), 5)
        self.assertEqual(sum(f["kind"] == "community_improvement" for f in DATA["features"]), 1)

    def test_traceability(self):
        sources = {s["id"] for s in DATA["sources"]}
        prefix = "https://github.com/LucasKazaki/AnimeRPG/blob/" + DATA["baseline_commit"] + "/"
        for feature in DATA["features"]:
            self.assertTrue(feature["sources"])
            self.assertTrue(set(feature["sources"]) <= sources)
            self.assertTrue(feature["evidence"])
            self.assertTrue(all(url.startswith(prefix) for url in feature["evidence"]))

    def test_acceptance_and_dependencies(self):
        ids = {f["id"] for f in DATA["features"]}
        for feature in DATA["features"]:
            self.assertGreaterEqual(len(feature["acceptance"]), 3)
            self.assertTrue(feature["dependencies"])
            for dependency in feature["dependencies"]:
                for reference in re.findall(r"(?:GAME|QOL)-\d{3}", dependency):
                    self.assertIn(reference, ids)

    def test_no_fabricated_implementation(self):
        for feature in DATA["features"]:
            self.assertEqual(feature["status"], "researched")
            self.assertIsNone(feature["implementation_commit"])
            self.assertEqual(feature["test_receipts"], [])
            self.assertFalse(feature["native_acceptance"])
            self.assertIsNone(feature["independent_review"])
            self.assertIsNone(feature["merge_sha"])
        for key, value in DATA["counts_at_setup"].items():
            self.assertEqual(value, 5 if key == "researched_reference" else 1 if key == "researched_community" else 0)

    def test_community_is_historical_not_consensus(self):
        source = next(s for s in DATA["sources"] if s["kind"] == "original_player_review")
        self.assertEqual(source["published"], "2025-04-01")
        self.assertEqual(source["current_resolution"], "unverified")
        self.assertFalse(source["independent_corroboration"])
        self.assertNotEqual(source["published"], source["accessed"])

    def test_authority_is_scoped(self):
        contract = FILES["Docs/Agents/ANIMERPG-HOURLY.md"]
        for needle in ("push and merge to main", "for this worker only", "independent implementation review", "Company Runtime", "expected-head-SHA", "zero new implemented gameplay features"):
            self.assertIn(needle, contract)

    def test_main_target(self):
        self.assertEqual(DATA["repository"], "LucasKazaki/AnimeRPG")
        self.assertEqual(DATA["target_branch"], "main")

    def test_hash_manifest(self):
        # The explicit commit is the manifest: a dirty/mismatched checkout fails.
        for path in PATHS:
            with self.subTest(path=path):
                committed = git("rev-parse", "--verify", f"{HEAD}:{path}").decode().strip()
                self.assertEqual(blob_sha((ROOT / path).read_bytes().replace(b"\r\n", b"\n")), committed)


def main() -> int:
    global BASE, HEAD, FILES, DATA
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--scope-base", required=True, help="Integration base commit/ref recorded in the setup receipt")
    parser.add_argument("--scope-head", default="HEAD", help="Exact setup candidate commit/ref; default HEAD")
    args = parser.parse_args()
    try:
        BASE = git("rev-parse", "--verify", "--end-of-options", args.scope_base + "^{commit}").decode().strip()
        HEAD = git("rev-parse", "--verify", "--end-of-options", args.scope_head + "^{commit}").decode().strip()
        git("merge-base", "--is-ancestor", BASE, HEAD)
        FILES = {path: (ROOT / path).read_bytes().replace(b"\r\n", b"\n").decode("utf-8") for path in PATHS}
        DATA = json.loads(FILES["Docs/Agents/animerpg-hourly/BACKLOG.json"])
    except (OSError, UnicodeError, ValueError, subprocess.SubprocessError) as exc:
        print(f"Setup validation prerequisites failed: {exc}", file=sys.stderr)
        return 2
    print(f"Setup scope base={BASE} head={HEAD}", flush=True)
    suite = unittest.defaultTestLoader.loadTestsFromTestCase(SetupChecks)
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    return 0 if result.wasSuccessful() else 1


if __name__ == "__main__":
    raise SystemExit(main())
