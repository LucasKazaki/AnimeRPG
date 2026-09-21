#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import json
import math
import random
import sys
import tempfile
import unittest
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import analyze_frame_timing_capture as analyzer


def write_csv(path: Path, values, *, warmup=2, max_samples=None, saturated=False,
              overrides=None, extra_metadata=None, frame_indices=None):
    values = list(values)
    if max_samples is None:
        max_samples = len(values)
    metadata = {
        "astral_frame_timing_schema": "1",
        "metric": "cpu_frame_interval_ms",
        "units": "milliseconds",
        "measurement_semantics": "wall_clock_interval_between_consecutive_Clock_Tick_calls",
        "timing_source": "Engine/Core/Clock.cpp:std::chrono::steady_clock",
        "gpu_timing": "unavailable",
        "acceptance_claim": "none",
        "warmup_frames": str(warmup),
        "max_samples": str(max_samples),
        "samples_saturated": "1" if saturated else "0",
    }
    if overrides:
        metadata.update(overrides)
    lines = [f"# {key}={value}" for key, value in metadata.items()]
    if extra_metadata:
        lines.extend(f"# {key}={value}" for key, value in extra_metadata.items())
    lines.append("frame_index,cpu_frame_interval_ms")
    if frame_indices is None:
        frame_indices = list(range(warmup, warmup + len(values)))
    for frame, value in zip(frame_indices, values):
        if isinstance(value, (int, float)) and math.isfinite(float(value)):
            rendered = f"{float(value):.6f}"
        else:
            rendered = str(value)
        lines.append(f"{frame},{rendered}")
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


class ParserTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.root = Path(self.tmp.name)

    def tearDown(self):
        self.tmp.cleanup()

    def test_known_percentiles_and_binding_fields(self):
        path = self.root / "timing.csv"
        write_csv(path, range(1, 101), warmup=120, max_samples=100, saturated=True)
        out = analyzer.parse_frame_timing_csv(path)
        stats = out["statistics_ms"]
        self.assertEqual(out["sample_count"], 100)
        self.assertTrue(out["sample_count_reached_limit"])
        self.assertAlmostEqual(stats["p50"], 50.5)
        self.assertAlmostEqual(stats["p95"], 95.05)
        self.assertAlmostEqual(stats["p99"], 99.01)
        self.assertAlmostEqual(stats["mean"], 50.5)
        self.assertEqual(out["start_frame_index"], 120)
        self.assertEqual(out["end_frame_index"], 219)

    def test_missing_duplicate_and_unknown_metadata_rejected(self):
        missing = self.root / "missing.csv"
        write_csv(missing, [16.0], overrides={"metric": ""})
        text = missing.read_text().replace("# metric=\n", "")
        missing.write_text(text)
        with self.assertRaises(analyzer.FrameTimingAnalysisError):
            analyzer.parse_frame_timing_csv(missing)

        duplicate = self.root / "dup.csv"
        write_csv(duplicate, [16.0])
        text = duplicate.read_text().replace("# units=milliseconds\n", "# units=milliseconds\n# units=milliseconds\n")
        duplicate.write_text(text)
        with self.assertRaises(analyzer.FrameTimingAnalysisError):
            analyzer.parse_frame_timing_csv(duplicate)

        extra = self.root / "extra.csv"
        write_csv(extra, [16.0], extra_metadata={"performance_budget_verified": "true"})
        with self.assertRaises(analyzer.FrameTimingAnalysisError):
            analyzer.parse_frame_timing_csv(extra)

    def test_fixed_metadata_and_claim_laundering_rejected(self):
        for key, bad in (("gpu_timing", "available"), ("acceptance_claim", "passed"), ("metric", "gpu_frame_time_ms")):
            path = self.root / f"{key}.csv"
            write_csv(path, [16.0], overrides={key: bad})
            with self.subTest(key=key), self.assertRaises(analyzer.FrameTimingAnalysisError):
                analyzer.parse_frame_timing_csv(path)

    def test_nonpositive_nonfinite_and_bad_numeric_rejected(self):
        for index, value in enumerate((0, -1, "nan", "inf", "oops")):
            path = self.root / f"bad-{index}.csv"
            write_csv(path, [value])
            with self.subTest(value=value), self.assertRaises(analyzer.FrameTimingAnalysisError):
                analyzer.parse_frame_timing_csv(path)

    def test_frame_indices_must_be_contiguous_and_after_warmup(self):
        before = self.root / "before.csv"
        write_csv(before, [1, 2], warmup=10, frame_indices=[9, 10])
        with self.assertRaises(analyzer.FrameTimingAnalysisError):
            analyzer.parse_frame_timing_csv(before)
        gap = self.root / "gap.csv"
        write_csv(gap, [1, 2], warmup=10, frame_indices=[10, 12])
        with self.assertRaises(analyzer.FrameTimingAnalysisError):
            analyzer.parse_frame_timing_csv(gap)
        reverse = self.root / "reverse.csv"
        write_csv(reverse, [1, 2], warmup=10, frame_indices=[11, 10])
        with self.assertRaises(analyzer.FrameTimingAnalysisError):
            analyzer.parse_frame_timing_csv(reverse)

    def test_sample_caps_and_saturation_contract(self):
        too_many = self.root / "too-many.csv"
        write_csv(too_many, [1, 2, 3], max_samples=2)
        with self.assertRaises(analyzer.FrameTimingAnalysisError):
            analyzer.parse_frame_timing_csv(too_many)
        fake_saturated = self.root / "fake-sat.csv"
        write_csv(fake_saturated, [1, 2], max_samples=3, saturated=True)
        with self.assertRaises(analyzer.FrameTimingAnalysisError):
            analyzer.parse_frame_timing_csv(fake_saturated)
        exact_unsat = self.root / "exact-unsat.csv"
        write_csv(exact_unsat, [1, 2], max_samples=2, saturated=False)
        out = analyzer.parse_frame_timing_csv(exact_unsat)
        self.assertTrue(out["sample_count_reached_limit"])
        self.assertFalse(out["source_metadata"]["samples_saturated"])

    def test_incomplete_capture_is_descriptive_not_acceptance(self):
        path = self.root / "partial-count.csv"
        write_csv(path, [10, 20, 30], max_samples=100, saturated=False)
        out = analyzer.parse_frame_timing_csv(path)
        self.assertFalse(out["sample_count_reached_limit"])
        self.assertEqual(out["sample_count"], 3)

    def test_seeded_mutations_fail_closed_or_revalidate(self):
        base = self.root / "mutation-base.csv"
        write_csv(base, [10 + i / 100 for i in range(50)], warmup=10, max_samples=50, saturated=True)
        original = base.read_text(encoding="utf-8")
        rng = random.Random(0xA57A1)
        alphabet = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_.,=# -"
        accepted = 0
        rejected = 0
        for index in range(250):
            chars = list(original)
            pos = rng.randrange(len(chars))
            replacement = rng.choice(alphabet)
            chars[pos] = replacement
            candidate = self.root / f"mutation-{index}.csv"
            candidate.write_text("".join(chars), encoding="utf-8")
            try:
                parsed = analyzer.parse_frame_timing_csv(candidate)
            except analyzer.FrameTimingAnalysisError:
                rejected += 1
                continue
            accepted += 1
            self.assertEqual(parsed["sample_count"], 50)
            self.assertEqual(parsed["source_metadata"]["metric"], "cpu_frame_interval_ms")
            self.assertEqual(parsed["source_metadata"]["gpu_timing"], "unavailable")
            self.assertEqual(parsed["source_metadata"]["acceptance_claim"], "none")
            self.assertGreater(parsed["statistics_ms"]["min"], 0.0)
            self.assertTrue(math.isfinite(parsed["statistics_ms"]["p99"]))
        self.assertGreater(rejected, 0)
        self.assertEqual(accepted + rejected, 250)

    def test_output_is_exclusive(self):
        output = self.root / "analysis.json"
        analyzer._write_new_json(output, {"ok": True})
        self.assertEqual(json.loads(output.read_text()), {"ok": True})
        with self.assertRaises(analyzer.FrameTimingAnalysisError):
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
        self.csv = self.evidence / "timing.csv"
        write_csv(self.csv, [10, 11, 12, 13], warmup=1, max_samples=4, saturated=True)
        import release_manifest
        self.release_manifest_mod = release_manifest
        self.release = self.root / "release.json"
        release_manifest.write_release_manifest(self.package, self.release, self.commit)
        spec = {
            "candidate": {"commit": self.commit, "build_config": "Release"},
            "workload": {"id": "fixture-idle-3d", "dimension": "3d", "fixture_description": "Procedural idle fixture for analyzer contract testing."},
            "run_protocol": {"width": 1920, "height": 1080, "window_mode": "windowed", "vsync": False, "warmup_seconds": 1, "sample_seconds": 5},
            "environment": {"os": "fixture", "cpu": "fixture", "logical_cpus": 4, "ram_bytes": 1024 * 1024 * 1024, "gpu": "fixture", "gpu_driver": "fixture"},
            "reference_versions": {"unreal": "5.8", "unity": "6.0"},
            "provenance": {"evidence_class": "hosted_contract_fixture", "machine_label": "fixture"},
            "evidence": [{"path": "timing.csv", "role": "cpu_frame_timing_csv"}],
        }
        self.benchmark = analyzer.benchmark_manifest.build_benchmark_manifest(spec, self.package, self.release, self.evidence)
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
        self.csv.write_text(self.csv.read_text() + "5,14.000000\n")
        with self.assertRaises(analyzer.FrameTimingAnalysisError):
            analyzer.analyze_bound_capture(
                self.benchmark, self.package, self.release, self.evidence,
                expected_commit=self.commit, expected_executable_sha256=self.exe_sha,
            )

    def test_expected_candidate_identity_is_required(self):
        with self.assertRaises(analyzer.FrameTimingAnalysisError):
            analyzer.analyze_bound_capture(
                self.benchmark, self.package, self.release, self.evidence,
                expected_commit="b" * 40, expected_executable_sha256=self.exe_sha,
            )
        with self.assertRaises(analyzer.FrameTimingAnalysisError):
            analyzer.analyze_bound_capture(
                self.benchmark, self.package, self.release, self.evidence,
                expected_commit=self.commit, expected_executable_sha256="0" * 64,
            )


if __name__ == "__main__":
    unittest.main(verbosity=2)
