#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import json
import random
import sys
import tempfile
import unittest
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import analyze_process_memory_capture as analyzer


def write_csv(
    path: Path,
    rows,
    *,
    warmup=2,
    stride=1,
    max_samples=None,
    saturated=False,
    sample_source="caller_supplied_contract_sample",
    overrides=None,
    extra_metadata=None,
):
    rows = list(rows)
    if max_samples is None:
        max_samples = len(rows)
    metadata = {
        "astral_process_memory_schema": "1",
        "metric": "process_os_memory_counters",
        "units": "bytes_except_page_fault_count",
        "sample_source": sample_source,
        "memory_scope": "current_process",
        "working_set_semantics": "resident_working_set_bytes",
        "private_usage_semantics": "process_commit_charge_bytes",
        "allocator_attribution": "unavailable",
        "vram": "unavailable",
        "leak_detection": "not_established",
        "performance_budget_claim": "none",
        "acceptance_claim": "none",
        "warmup_frames": str(warmup),
        "sample_every_frames": str(stride),
        "max_samples": str(max_samples),
        "samples_saturated": "1" if saturated else "0",
    }
    if overrides:
        metadata.update(overrides)
    lines = [f"# {key}={value}" for key, value in metadata.items()]
    if extra_metadata:
        lines.extend(f"# {key}={value}" for key, value in extra_metadata.items())
    lines.append("frame_index,working_set_bytes,peak_working_set_bytes,private_usage_bytes,page_fault_count")
    for row in rows:
        lines.append(",".join(str(value) for value in row))
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def linear_rows(count=100, *, warmup=120, stride=1):
    rows = []
    for i in range(count):
        frame = warmup + i * stride
        working = 1_000 + i * 10
        peak = 2_000 + i * 10
        private = 3_000 + i * 20
        faults = 100 + i * 2
        rows.append((frame, working, peak, private, faults))
    return rows


class ParserTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.root = Path(self.tmp.name)

    def tearDown(self):
        self.tmp.cleanup()

    def test_known_statistics_windows_and_frame_trends(self):
        path = self.root / "memory.csv"
        write_csv(path, linear_rows(), warmup=120, max_samples=100, saturated=True)
        out = analyzer.parse_process_memory_csv(path)
        self.assertEqual(out["sample_count"], 100)
        self.assertTrue(out["sample_count_reached_limit"])
        self.assertEqual(out["start_frame_index"], 120)
        self.assertEqual(out["end_frame_index"], 219)
        working = out["statistics"]["working_set_bytes"]
        private = out["statistics"]["private_usage_bytes"]
        faults = out["statistics"]["page_fault_count"]
        self.assertAlmostEqual(working["p50"], 1495.0)
        self.assertAlmostEqual(working["p95"], 1940.5)
        self.assertAlmostEqual(working["p99"], 1980.1)
        self.assertAlmostEqual(working["ols_slope_per_1000_frames"], 10_000.0)
        self.assertAlmostEqual(private["ols_slope_per_1000_frames"], 20_000.0)
        self.assertEqual(faults["first"], 100)
        self.assertEqual(faults["last"], 298)
        self.assertEqual(faults["delta"], 198)
        self.assertAlmostEqual(faults["ols_slope_per_1000_frames"], 2_000.0)
        self.assertEqual(out["first_last_window_fraction"], 0.10)

    def test_missing_duplicate_and_unknown_metadata_rejected(self):
        missing = self.root / "missing.csv"
        write_csv(missing, linear_rows(1, warmup=2), warmup=2)
        missing.write_text(missing.read_text().replace("# metric=process_os_memory_counters\n", ""))
        with self.assertRaises(analyzer.ProcessMemoryAnalysisError):
            analyzer.parse_process_memory_csv(missing)

        duplicate = self.root / "duplicate.csv"
        write_csv(duplicate, linear_rows(1, warmup=2), warmup=2)
        duplicate.write_text(duplicate.read_text().replace(
            "# units=bytes_except_page_fault_count\n",
            "# units=bytes_except_page_fault_count\n# units=bytes_except_page_fault_count\n",
        ))
        with self.assertRaises(analyzer.ProcessMemoryAnalysisError):
            analyzer.parse_process_memory_csv(duplicate)

        extra = self.root / "extra.csv"
        write_csv(extra, linear_rows(1, warmup=2), warmup=2,
                  extra_metadata={"ram_budget_verified": "true"})
        with self.assertRaises(analyzer.ProcessMemoryAnalysisError):
            analyzer.parse_process_memory_csv(extra)

    def test_fixed_claim_metadata_and_sample_source_rejected_when_changed(self):
        cases = (
            ("vram", "available"),
            ("leak_detection", "verified"),
            ("performance_budget_claim", "passed"),
            ("acceptance_claim", "passed"),
            ("metric", "allocator_memory"),
            ("sample_source", "injected_native_claim"),
        )
        for key, bad in cases:
            path = self.root / f"bad-{key}.csv"
            write_csv(path, linear_rows(1, warmup=2), warmup=2, overrides={key: bad})
            with self.subTest(key=key), self.assertRaises(analyzer.ProcessMemoryAnalysisError):
                analyzer.parse_process_memory_csv(path)

    def test_noncanonical_or_invalid_unsigned_values_rejected(self):
        base = linear_rows(1, warmup=2)[0]
        variants = [
            ("leading-zero", ("02", *base[1:])),
            ("negative", (2, -1, base[2], base[3], base[4])),
            ("float", (2, "1000.0", base[2], base[3], base[4])),
            ("text", (2, "oops", base[2], base[3], base[4])),
        ]
        for label, row in variants:
            path = self.root / f"{label}.csv"
            write_csv(path, [row], warmup=2)
            with self.subTest(label=label), self.assertRaises(analyzer.ProcessMemoryAnalysisError):
                analyzer.parse_process_memory_csv(path)

    def test_frame_indices_must_begin_at_warmup_and_follow_stride(self):
        early = self.root / "early.csv"
        write_csv(early, [(9, 10, 20, 30, 40)], warmup=10)
        with self.assertRaises(analyzer.ProcessMemoryAnalysisError):
            analyzer.parse_process_memory_csv(early)

        gap = self.root / "gap.csv"
        write_csv(gap, [(10, 10, 20, 30, 40), (14, 11, 21, 31, 41)], warmup=10, stride=2)
        with self.assertRaises(analyzer.ProcessMemoryAnalysisError):
            analyzer.parse_process_memory_csv(gap)

        valid = self.root / "valid-stride.csv"
        write_csv(valid, [(10, 10, 20, 30, 40), (12, 11, 21, 31, 41)], warmup=10, stride=2)
        out = analyzer.parse_process_memory_csv(valid)
        self.assertEqual(out["end_frame_index"], 12)
        self.assertEqual(out["source_metadata"]["sample_every_frames"], 2)

    def test_peak_working_set_relationship_and_monotonicity(self):
        less_than_current = self.root / "peak-less.csv"
        write_csv(less_than_current, [(2, 100, 99, 200, 1)], warmup=2)
        with self.assertRaises(analyzer.ProcessMemoryAnalysisError):
            analyzer.parse_process_memory_csv(less_than_current)

        decreasing_peak = self.root / "peak-decrease.csv"
        write_csv(decreasing_peak, [(2, 100, 150, 200, 1), (3, 90, 149, 201, 2)], warmup=2)
        with self.assertRaises(analyzer.ProcessMemoryAnalysisError):
            analyzer.parse_process_memory_csv(decreasing_peak)

    def test_page_fault_count_must_not_decrease(self):
        path = self.root / "fault-decrease.csv"
        write_csv(path, [(2, 100, 150, 200, 2), (3, 101, 151, 201, 1)], warmup=2)
        with self.assertRaises(analyzer.ProcessMemoryAnalysisError):
            analyzer.parse_process_memory_csv(path)

    def test_sample_caps_saturation_and_incomplete_capture_remain_descriptive(self):
        too_many = self.root / "too-many.csv"
        write_csv(too_many, linear_rows(3, warmup=2), warmup=2, max_samples=2)
        with self.assertRaises(analyzer.ProcessMemoryAnalysisError):
            analyzer.parse_process_memory_csv(too_many)

        fake_saturated = self.root / "fake-sat.csv"
        write_csv(fake_saturated, linear_rows(2, warmup=2), warmup=2, max_samples=3, saturated=True)
        with self.assertRaises(analyzer.ProcessMemoryAnalysisError):
            analyzer.parse_process_memory_csv(fake_saturated)

        incomplete = self.root / "incomplete.csv"
        write_csv(incomplete, linear_rows(3, warmup=2), warmup=2, max_samples=100, saturated=False)
        out = analyzer.parse_process_memory_csv(incomplete)
        self.assertFalse(out["sample_count_reached_limit"])
        self.assertEqual(out["sample_count"], 3)

    def test_seeded_mutations_fail_closed_or_fully_revalidate(self):
        base = self.root / "mutation-base.csv"
        write_csv(base, linear_rows(50, warmup=10), warmup=10, max_samples=50, saturated=True)
        original = base.read_text(encoding="utf-8")
        rng = random.Random(0xA57A1)
        alphabet = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_.,=# -"
        accepted = rejected = 0
        for index in range(250):
            chars = list(original)
            pos = rng.randrange(len(chars))
            chars[pos] = rng.choice(alphabet)
            candidate = self.root / f"mutation-{index}.csv"
            candidate.write_text("".join(chars), encoding="utf-8")
            try:
                parsed = analyzer.parse_process_memory_csv(candidate)
            except analyzer.ProcessMemoryAnalysisError:
                rejected += 1
                continue
            accepted += 1
            self.assertEqual(parsed["sample_count"], 50)
            self.assertEqual(parsed["source_metadata"]["metric"], "process_os_memory_counters")
            self.assertEqual(parsed["source_metadata"]["vram"], "unavailable")
            self.assertEqual(parsed["source_metadata"]["leak_detection"], "not_established")
            self.assertEqual(parsed["source_metadata"]["acceptance_claim"], "none")
            self.assertGreaterEqual(parsed["statistics"]["working_set_bytes"]["min"], 0)
        self.assertGreater(rejected, 0)
        self.assertEqual(accepted + rejected, 250)

    def test_output_is_exclusive_and_acceptance_stays_false(self):
        output = self.root / "analysis.json"
        analyzer._write_new_json(output, {"acceptance": analyzer.ACCEPTANCE})
        written = json.loads(output.read_text())
        self.assertTrue(all(value is False for value in written["acceptance"].values()))
        with self.assertRaises(analyzer.ProcessMemoryAnalysisError):
            analyzer._write_new_json(output, {"ok": False})


@unittest.skipIf(analyzer.benchmark_manifest is None, "repository benchmark_manifest module unavailable in partial fixture")
class ProductionBindingTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.root = Path(self.tmp.name)
        self.package = self.root / "package"
        self.evidence = self.root / "evidence"
        self.package.mkdir(); self.evidence.mkdir()
        self.commit = "a" * 40
        (self.package / "AstralGame.exe").write_bytes(b"MZ bounded Astral fixture\n")
        self.csv = self.evidence / "process-memory.csv"
        write_csv(self.csv, linear_rows(4, warmup=1), warmup=1, max_samples=4, saturated=True)
        import release_manifest
        self.release = self.root / "release.json"
        release_manifest.write_release_manifest(self.package, self.release, self.commit)
        spec = {
            "candidate": {"commit": self.commit, "build_config": "Release"},
            "workload": {"id": "fixture-idle-3d", "dimension": "3d", "fixture_description": "Procedural idle fixture for process-memory analyzer contract testing."},
            "run_protocol": {"width": 1920, "height": 1080, "window_mode": "windowed", "vsync": False, "warmup_seconds": 1, "sample_seconds": 5},
            "environment": {"os": "fixture", "cpu": "fixture", "logical_cpus": 4, "ram_bytes": 1024 * 1024 * 1024, "gpu": "fixture", "gpu_driver": "fixture"},
            "reference_versions": {"unreal": "5.8", "unity": "6.0"},
            "provenance": {"evidence_class": "hosted_contract_fixture", "machine_label": "fixture"},
            "evidence": [{"path": "process-memory.csv", "role": "process_memory_csv"}],
        }
        self.benchmark = analyzer.benchmark_manifest.build_benchmark_manifest(
            spec, self.package, self.release, self.evidence
        )
        self.exe_sha = self.benchmark["candidate"]["executable_sha256"]

    def tearDown(self):
        self.tmp.cleanup()

    def test_real_production_manifest_binding(self):
        result = analyzer.analyze_bound_capture(
            self.benchmark, self.package, self.release, self.evidence,
            expected_commit=self.commit, expected_executable_sha256=self.exe_sha,
        )
        self.assertEqual(result["capture"]["sample_count"], 4)
        self.assertEqual(result["candidate_commit"], self.commit)
        self.assertEqual(result["source_csv"]["sha256"], hashlib.sha256(self.csv.read_bytes()).hexdigest())
        self.assertTrue(all(value is False for value in result["acceptance"].values()))

    def test_tampered_raw_evidence_rejected_by_production_verifier(self):
        self.csv.write_text(self.csv.read_text() + "5,1040,2040,3080,108\n")
        with self.assertRaises(analyzer.ProcessMemoryAnalysisError):
            analyzer.analyze_bound_capture(
                self.benchmark, self.package, self.release, self.evidence,
                expected_commit=self.commit, expected_executable_sha256=self.exe_sha,
            )

    def test_expected_candidate_identity_is_required(self):
        with self.assertRaises(analyzer.ProcessMemoryAnalysisError):
            analyzer.analyze_bound_capture(
                self.benchmark, self.package, self.release, self.evidence,
                expected_commit="b" * 40, expected_executable_sha256=self.exe_sha,
            )
        with self.assertRaises(analyzer.ProcessMemoryAnalysisError):
            analyzer.analyze_bound_capture(
                self.benchmark, self.package, self.release, self.evidence,
                expected_commit=self.commit, expected_executable_sha256="0" * 64,
            )


if __name__ == "__main__":
    unittest.main(verbosity=2)
