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
import analyze_frame_phase_timing_capture as analyzer


def write_csv(
    path: Path,
    rows,
    *,
    warmup=2,
    max_samples=None,
    saturated=False,
    overrides=None,
    extra_metadata=None,
    frame_indices=None,
):
    rows = list(rows)
    if max_samples is None:
        max_samples = len(rows)
    metadata = {
        "astral_frame_phase_timing_schema": "1",
        "metric": "main_thread_phase_wall_ms",
        "units": "milliseconds",
        "measurement_semantics": "wall_clock_intervals_for_contiguous_Win32_main_loop_phases",
        "timing_source": "std::chrono::steady_clock",
        "phase_order": "message_pump,update_control,render_submit,frame_wait",
        "cpu_scope": "Win32_main_thread_only",
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
    lines.append(analyzer.CSV_HEADER)
    if frame_indices is None:
        frame_indices = list(range(warmup, warmup + len(rows)))
    for frame, row in zip(frame_indices, rows):
        rendered = []
        for value in row:
            if isinstance(value, (int, float)):
                rendered.append(f"{float(value):.6f}")
            else:
                rendered.append(str(value))
        lines.append(f"{frame}," + ",".join(rendered))
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def constant_rows(count: int, values=(1.0, 2.0, 3.0, 4.0, 10.0)):
    return [values for _ in range(count)]


class ParserTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.root = Path(self.tmp.name)

    def tearDown(self):
        self.tmp.cleanup()

    def test_known_phase_statistics_and_shares(self):
        path = self.root / "phase.csv"
        write_csv(path, constant_rows(100), warmup=120, max_samples=100, saturated=True)
        out = analyzer.parse_frame_phase_timing_csv(path)
        self.assertEqual(out["sample_count"], 100)
        self.assertTrue(out["sample_count_reached_limit"])
        self.assertEqual(out["start_frame_index"], 120)
        self.assertEqual(out["end_frame_index"], 219)
        self.assertAlmostEqual(out["statistics_ms"]["message_pump_ms"]["p99"], 1.0)
        self.assertAlmostEqual(out["statistics_ms"]["render_submit_ms"]["mean"], 3.0)
        self.assertAlmostEqual(out["statistics_ms"]["loop_total_ms"]["sum"], 1000.0)
        shares = out["aggregate_phase_share_of_loop_total"]
        self.assertAlmostEqual(shares["message_pump_ms"], 0.1)
        self.assertAlmostEqual(shares["update_control_ms"], 0.2)
        self.assertAlmostEqual(shares["render_submit_ms"], 0.3)
        self.assertAlmostEqual(shares["frame_wait_ms"], 0.4)
        self.assertAlmostEqual(sum(shares.values()), 1.0)

    def test_row_total_rounding_tolerance_is_bounded(self):
        accepted = self.root / "accepted.csv"
        write_csv(accepted, [(1.0, 1.0, 1.0, 1.0, 4.000003)])
        parsed = analyzer.parse_frame_phase_timing_csv(accepted)
        self.assertEqual(
            parsed["row_total_consistency"]["max_absolute_serialized_error_microseconds"], 3
        )

        rejected = self.root / "rejected.csv"
        write_csv(rejected, [(1.0, 1.0, 1.0, 1.0, 4.000004)])
        with self.assertRaises(analyzer.FramePhaseTimingAnalysisError):
            analyzer.parse_frame_phase_timing_csv(rejected)

    def test_missing_duplicate_unknown_metadata_rejected(self):
        missing = self.root / "missing.csv"
        write_csv(missing, constant_rows(1))
        missing.write_text(
            missing.read_text(encoding="utf-8").replace("# metric=main_thread_phase_wall_ms\n", ""),
            encoding="utf-8",
        )
        with self.assertRaises(analyzer.FramePhaseTimingAnalysisError):
            analyzer.parse_frame_phase_timing_csv(missing)

        duplicate = self.root / "duplicate.csv"
        write_csv(duplicate, constant_rows(1))
        duplicate.write_text(
            duplicate.read_text(encoding="utf-8").replace(
                "# units=milliseconds\n", "# units=milliseconds\n# units=milliseconds\n"
            ),
            encoding="utf-8",
        )
        with self.assertRaises(analyzer.FramePhaseTimingAnalysisError):
            analyzer.parse_frame_phase_timing_csv(duplicate)

        extra = self.root / "extra.csv"
        write_csv(extra, constant_rows(1), extra_metadata={"performance_budget_verified": "true"})
        with self.assertRaises(analyzer.FramePhaseTimingAnalysisError):
            analyzer.parse_frame_phase_timing_csv(extra)

    def test_fixed_scope_and_claim_laundering_rejected(self):
        cases = (
            ("gpu_timing", "available"),
            ("acceptance_claim", "passed"),
            ("cpu_scope", "all_threads"),
            ("metric", "gpu_frame_time_ms"),
            ("phase_order", "render_submit,message_pump,update_control,frame_wait"),
        )
        for key, bad in cases:
            path = self.root / f"{key}.csv"
            write_csv(path, constant_rows(1), overrides={key: bad})
            with self.subTest(key=key), self.assertRaises(analyzer.FramePhaseTimingAnalysisError):
                analyzer.parse_frame_phase_timing_csv(path)

    def test_bad_numeric_and_nonpositive_total_rejected(self):
        cases = (
            ("nan", 1, 1, 1, 3),
            (-1, 1, 1, 1, 2),
            (1, 1, 1, 1, 0),
            (1, 1, 1, 1, "inf"),
            (1, 1, 1, 1, "oops"),
        )
        for index, row in enumerate(cases):
            path = self.root / f"bad-{index}.csv"
            write_csv(path, [row])
            with self.subTest(row=row), self.assertRaises(analyzer.FramePhaseTimingAnalysisError):
                analyzer.parse_frame_phase_timing_csv(path)

    def test_frame_indices_must_be_contiguous_and_after_warmup(self):
        before = self.root / "before.csv"
        write_csv(before, constant_rows(2), warmup=10, frame_indices=[9, 10])
        with self.assertRaises(analyzer.FramePhaseTimingAnalysisError):
            analyzer.parse_frame_phase_timing_csv(before)
        gap = self.root / "gap.csv"
        write_csv(gap, constant_rows(2), warmup=10, frame_indices=[10, 12])
        with self.assertRaises(analyzer.FramePhaseTimingAnalysisError):
            analyzer.parse_frame_phase_timing_csv(gap)
        reverse = self.root / "reverse.csv"
        write_csv(reverse, constant_rows(2), warmup=10, frame_indices=[11, 10])
        with self.assertRaises(analyzer.FramePhaseTimingAnalysisError):
            analyzer.parse_frame_phase_timing_csv(reverse)

    def test_sample_caps_and_saturation_contract(self):
        too_many = self.root / "too-many.csv"
        write_csv(too_many, constant_rows(3), max_samples=2)
        with self.assertRaises(analyzer.FramePhaseTimingAnalysisError):
            analyzer.parse_frame_phase_timing_csv(too_many)
        fake_saturated = self.root / "fake-sat.csv"
        write_csv(fake_saturated, constant_rows(2), max_samples=3, saturated=True)
        with self.assertRaises(analyzer.FramePhaseTimingAnalysisError):
            analyzer.parse_frame_phase_timing_csv(fake_saturated)
        exact_unsat = self.root / "exact-unsat.csv"
        write_csv(exact_unsat, constant_rows(2), max_samples=2, saturated=False)
        out = analyzer.parse_frame_phase_timing_csv(exact_unsat)
        self.assertTrue(out["sample_count_reached_limit"])
        self.assertFalse(out["source_metadata"]["samples_saturated"])

    def test_partial_capture_remains_descriptive_only(self):
        path = self.root / "partial.csv"
        write_csv(path, constant_rows(3), max_samples=100, saturated=False)
        out = analyzer.parse_frame_phase_timing_csv(path)
        self.assertFalse(out["sample_count_reached_limit"])
        self.assertEqual(out["sample_count"], 3)

    def test_seeded_mutations_fail_closed_or_revalidate(self):
        base = self.root / "mutation-base.csv"
        rows = []
        for i in range(50):
            a = 0.5 + i / 100.0
            b = 1.0 + i / 200.0
            c = 1.5 + i / 300.0
            d = 2.0 + i / 400.0
            rows.append((a, b, c, d, a + b + c + d))
        write_csv(base, rows, warmup=10, max_samples=50, saturated=True)
        original = base.read_text(encoding="utf-8")
        rng = random.Random(0xA57A2)
        alphabet = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_.,=# -"
        accepted = 0
        rejected = 0
        for index in range(250):
            chars = list(original)
            pos = rng.randrange(len(chars))
            chars[pos] = rng.choice(alphabet)
            candidate = self.root / f"mutation-{index}.csv"
            candidate.write_text("".join(chars), encoding="utf-8")
            try:
                parsed = analyzer.parse_frame_phase_timing_csv(candidate)
            except analyzer.FramePhaseTimingAnalysisError:
                rejected += 1
                continue
            accepted += 1
            self.assertEqual(parsed["sample_count"], 50)
            self.assertEqual(parsed["source_metadata"]["cpu_scope"], "Win32_main_thread_only")
            self.assertEqual(parsed["source_metadata"]["gpu_timing"], "unavailable")
            self.assertEqual(parsed["source_metadata"]["acceptance_claim"], "none")
            self.assertLessEqual(
                parsed["row_total_consistency"]["max_absolute_serialized_error_microseconds"],
                analyzer.ROW_TOTAL_ROUNDING_TOLERANCE_US,
            )
        self.assertGreater(rejected, 0)
        self.assertEqual(accepted + rejected, 250)

    def test_output_is_exclusive(self):
        output = self.root / "analysis.json"
        analyzer._write_new_json(output, {"ok": True})
        self.assertEqual(json.loads(output.read_text()), {"ok": True})
        with self.assertRaises(analyzer.FramePhaseTimingAnalysisError):
            analyzer._write_new_json(output, {"ok": False})


@unittest.skipIf(analyzer.benchmark_manifest is None, "repository benchmark_manifest module unavailable in partial fixture")
class ProductionBindingTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.root = Path(self.tmp.name)
        self.package = self.root / "package"
        self.evidence = self.root / "evidence"
        self.package.mkdir()
        self.evidence.mkdir()
        self.commit = "a" * 40
        (self.package / "AstralGame.exe").write_bytes(b"MZ bounded Astral phase fixture\n")
        self.csv = self.evidence / "phase.csv"
        write_csv(self.csv, constant_rows(4), warmup=1, max_samples=4, saturated=True)
        import release_manifest

        self.release = self.root / "release.json"
        release_manifest.write_release_manifest(self.package, self.release, self.commit)
        spec = {
            "candidate": {"commit": self.commit, "build_config": "Release"},
            "workload": {
                "id": "fixture-idle-3d",
                "dimension": "3d",
                "fixture_description": "Procedural idle fixture for phase analyzer contract testing.",
            },
            "run_protocol": {
                "width": 1920,
                "height": 1080,
                "window_mode": "windowed",
                "vsync": False,
                "warmup_seconds": 1,
                "sample_seconds": 5,
            },
            "environment": {
                "os": "fixture",
                "cpu": "fixture",
                "logical_cpus": 4,
                "ram_bytes": 1024 * 1024 * 1024,
                "gpu": "fixture",
                "gpu_driver": "fixture",
            },
            "reference_versions": {"unreal": "5.8", "unity": "6.0"},
            "provenance": {
                "evidence_class": "hosted_contract_fixture",
                "machine_label": "fixture",
            },
            "evidence": [{"path": "phase.csv", "role": "cpu_phase_timing_csv"}],
        }
        builder = getattr(analyzer.benchmark_manifest, "build_benchmark_manifest", None)
        if builder is None:
            builder = analyzer.benchmark_manifest.build_manifest
        self.benchmark = builder(spec, self.package, self.release, self.evidence)
        self.exe_sha = self.benchmark["candidate"]["executable_sha256"]

    def tearDown(self):
        self.tmp.cleanup()

    def test_real_production_manifest_binding(self):
        result = analyzer.analyze_bound_capture(
            self.benchmark,
            self.package,
            self.release,
            self.evidence,
            expected_commit=self.commit,
            expected_executable_sha256=self.exe_sha,
        )
        self.assertEqual(result["capture"]["sample_count"], 4)
        self.assertEqual(result["candidate_commit"], self.commit)
        self.assertEqual(
            result["source_csv"]["sha256"], hashlib.sha256(self.csv.read_bytes()).hexdigest()
        )
        self.assertTrue(all(value is False for value in result["acceptance"].values()))

    def test_tampered_raw_evidence_rejected_by_production_verifier(self):
        self.csv.write_text(self.csv.read_text() + "5,1.000000,2.000000,3.000000,4.000000,10.000000\n")
        with self.assertRaises(analyzer.FramePhaseTimingAnalysisError):
            analyzer.analyze_bound_capture(
                self.benchmark,
                self.package,
                self.release,
                self.evidence,
                expected_commit=self.commit,
                expected_executable_sha256=self.exe_sha,
            )

    def test_expected_candidate_identity_is_required(self):
        with self.assertRaises(analyzer.FramePhaseTimingAnalysisError):
            analyzer.analyze_bound_capture(
                self.benchmark,
                self.package,
                self.release,
                self.evidence,
                expected_commit="b" * 40,
                expected_executable_sha256=self.exe_sha,
            )
        with self.assertRaises(analyzer.FramePhaseTimingAnalysisError):
            analyzer.analyze_bound_capture(
                self.benchmark,
                self.package,
                self.release,
                self.evidence,
                expected_commit=self.commit,
                expected_executable_sha256="0" * 64,
            )


if __name__ == "__main__":
    unittest.main(verbosity=2)
