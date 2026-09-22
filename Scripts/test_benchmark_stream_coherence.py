#!/usr/bin/env python3
from __future__ import annotations

import json
import sys
import tempfile
import unittest
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import verify_benchmark_stream_coherence as verifier


def capture_triplet(*, count=4, warmup=2, memory_stride=1, memory_count=None):
    end = warmup + count - 1
    if memory_count is None:
        memory_count = ((end - warmup) // memory_stride) + 1
    memory_end = warmup + (memory_count - 1) * memory_stride
    whole = {
        "source_metadata": {
            "warmup_frames": warmup,
            "max_samples": count,
            "samples_saturated": True,
        },
        "sample_count": count,
        "sample_count_reached_limit": True,
        "start_frame_index": warmup,
        "end_frame_index": end,
    }
    phases = {
        "source_metadata": {
            "warmup_frames": warmup,
            "max_samples": count,
            "samples_saturated": True,
        },
        "sample_count": count,
        "sample_count_reached_limit": True,
        "start_frame_index": warmup,
        "end_frame_index": end,
    }
    memory = {
        "source_metadata": {
            "warmup_frames": warmup,
            "sample_every_frames": memory_stride,
            "max_samples": memory_count,
            "samples_saturated": True,
            "sample_source": "caller_supplied_contract_sample",
        },
        "sample_count": memory_count,
        "sample_count_reached_limit": True,
        "start_frame_index": warmup,
        "end_frame_index": memory_end,
    }
    return whole, phases, memory


class AlignmentTests(unittest.TestCase):
    def test_matching_streams_verify(self):
        whole, phases, memory = capture_triplet(count=6, warmup=120)
        result = verifier.verify_capture_alignment(whole, phases, memory)
        self.assertEqual(result["warmup_frames"], 120)
        self.assertEqual(result["measured_frame_interval"]["frame_count"], 6)
        self.assertEqual(result["process_memory"]["expected_sample_count_for_interval"], 6)

    def test_memory_stride_can_downsample_complete_interval(self):
        whole, phases, memory = capture_triplet(count=6, warmup=120, memory_stride=2)
        result = verifier.verify_capture_alignment(whole, phases, memory)
        self.assertEqual(result["process_memory"]["sample_count"], 3)
        self.assertEqual(result["process_memory"]["end_frame_index"], 124)
        self.assertEqual(result["measured_frame_interval"]["end_frame_index"], 125)

    def test_warmup_mismatch_is_rejected(self):
        whole, phases, memory = capture_triplet()
        phases["source_metadata"]["warmup_frames"] = 3
        with self.assertRaises(verifier.BenchmarkStreamCoherenceError):
            verifier.verify_capture_alignment(whole, phases, memory)

    def test_phase_interval_mismatch_is_rejected(self):
        whole, phases, memory = capture_triplet(count=4)
        phases["sample_count"] = 3
        phases["end_frame_index"] = 4
        with self.assertRaises(verifier.BenchmarkStreamCoherenceError):
            verifier.verify_capture_alignment(whole, phases, memory)

    def test_memory_missing_scheduled_samples_is_rejected(self):
        whole, phases, memory = capture_triplet(count=6, memory_stride=2, memory_count=2)
        with self.assertRaises(verifier.BenchmarkStreamCoherenceError):
            verifier.verify_capture_alignment(whole, phases, memory)

    def test_timing_capture_limits_must_match(self):
        whole, phases, memory = capture_triplet(count=4)
        phases["source_metadata"]["max_samples"] = 5
        with self.assertRaises(verifier.BenchmarkStreamCoherenceError):
            verifier.verify_capture_alignment(whole, phases, memory)

    def test_native_provenance_requires_production_memory_source(self):
        _, _, memory = capture_triplet()
        with self.assertRaises(verifier.BenchmarkStreamCoherenceError):
            verifier.validate_memory_source_for_provenance(memory, "local_native")
        memory["source_metadata"]["sample_source"] = "Windows_GetProcessMemoryInfo_PROCESS_MEMORY_COUNTERS_EX"
        verifier.validate_memory_source_for_provenance(memory, "local_native")

    def test_output_is_exclusive_and_acceptance_is_false(self):
        with tempfile.TemporaryDirectory() as tmp:
            output = Path(tmp) / "coherence.json"
            verifier._write_new_json(output, {"acceptance": verifier.ACCEPTANCE})
            value = json.loads(output.read_text(encoding="utf-8"))
            self.assertTrue(all(item is False for item in value["acceptance"].values()))
            with self.assertRaises(verifier.BenchmarkStreamCoherenceError):
                verifier._write_new_json(output, {"ok": False})


MODULES_AVAILABLE = all(
    module is not None for module in (
        verifier.benchmark_manifest,
        verifier.analyze_frame_timing_capture,
        verifier.analyze_frame_phase_timing_capture,
        verifier.analyze_process_memory_capture,
    )
)


@unittest.skipUnless(MODULES_AVAILABLE, "repository profiling/manifest modules unavailable in partial fixture")
class ProductionBindingTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.root = Path(self.tmp.name)
        self.package = self.root / "package"
        self.evidence = self.root / "evidence"
        self.package.mkdir()
        self.evidence.mkdir()
        self.commit = "a" * 40
        (self.package / "AstralGame.exe").write_bytes(b"MZ Astral stream coherence fixture\n")
        self.whole = self.evidence / "whole.csv"
        self.phase = self.evidence / "phase.csv"
        self.memory = self.evidence / "memory.csv"
        self._write_whole(self.whole, count=4, warmup=2)
        self._write_phase(self.phase, count=4, warmup=2)
        self._write_memory(self.memory, count=4, warmup=2, source="caller_supplied_contract_sample")

        import release_manifest
        self.release = self.root / "release.json"
        release_manifest.write_release_manifest(self.package, self.release, self.commit)
        self.spec = {
            "candidate": {"commit": self.commit, "build_config": "Release"},
            "workload": {
                "id": "fixture-idle-3d",
                "dimension": "3d",
                "fixture_description": "Procedural fixture for cross-stream coherence contract testing.",
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
            "provenance": {"evidence_class": "hosted_contract_fixture", "machine_label": "fixture"},
            "evidence": [
                {"path": "whole.csv", "role": "cpu_frame_timing_csv"},
                {"path": "phase.csv", "role": "cpu_phase_timing_csv"},
                {"path": "memory.csv", "role": "process_memory_csv"},
            ],
        }
        self.benchmark = verifier.benchmark_manifest.build_benchmark_manifest(
            self.spec, self.package, self.release, self.evidence
        )
        self.exe_sha = self.benchmark["candidate"]["executable_sha256"]

    def tearDown(self):
        self.tmp.cleanup()

    @staticmethod
    def _write_whole(path: Path, *, count: int, warmup: int):
        lines = [
            "# astral_frame_timing_schema=1",
            "# metric=cpu_frame_interval_ms",
            "# units=milliseconds",
            "# measurement_semantics=wall_clock_interval_between_consecutive_Clock_Tick_calls",
            "# timing_source=Engine/Core/Clock.cpp:std::chrono::steady_clock",
            "# gpu_timing=unavailable",
            "# acceptance_claim=none",
            f"# warmup_frames={warmup}",
            f"# max_samples={count}",
            "# samples_saturated=1",
            "frame_index,cpu_frame_interval_ms",
        ]
        lines.extend(f"{warmup + i},{16.0 + i / 10:.6f}" for i in range(count))
        path.write_text("\n".join(lines) + "\n", encoding="utf-8")

    @staticmethod
    def _write_phase(path: Path, *, count: int, warmup: int):
        lines = [
            "# astral_frame_phase_timing_schema=1",
            "# metric=main_thread_phase_wall_ms",
            "# units=milliseconds",
            "# measurement_semantics=wall_clock_intervals_for_contiguous_Win32_main_loop_phases",
            "# timing_source=std::chrono::steady_clock",
            "# phase_order=message_pump,update_control,render_submit,frame_wait",
            "# cpu_scope=Win32_main_thread_only",
            "# gpu_timing=unavailable",
            "# acceptance_claim=none",
            f"# warmup_frames={warmup}",
            f"# max_samples={count}",
            "# samples_saturated=1",
            "frame_index,message_pump_ms,update_control_ms,render_submit_ms,frame_wait_ms,loop_total_ms",
        ]
        for i in range(count):
            lines.append(f"{warmup + i},1.000000,2.000000,3.000000,10.000000,16.000000")
        path.write_text("\n".join(lines) + "\n", encoding="utf-8")

    @staticmethod
    def _write_memory(path: Path, *, count: int, warmup: int, source: str, stride: int = 1):
        lines = [
            "# astral_process_memory_schema=1",
            "# metric=process_os_memory_counters",
            "# units=bytes_except_page_fault_count",
            f"# sample_source={source}",
            "# memory_scope=current_process",
            "# working_set_semantics=resident_working_set_bytes",
            "# private_usage_semantics=process_commit_charge_bytes",
            "# allocator_attribution=unavailable",
            "# vram=unavailable",
            "# leak_detection=not_established",
            "# performance_budget_claim=none",
            "# acceptance_claim=none",
            f"# warmup_frames={warmup}",
            f"# sample_every_frames={stride}",
            f"# max_samples={count}",
            "# samples_saturated=1",
            "frame_index,working_set_bytes,peak_working_set_bytes,private_usage_bytes,page_fault_count",
        ]
        for i in range(count):
            lines.append(f"{warmup + i * stride},{1000+i},{2000+i},{3000+i},{100+i}")
        path.write_text("\n".join(lines) + "\n", encoding="utf-8")

    def _verify(self, benchmark=None):
        return verifier.verify_bound_stream_coherence(
            benchmark or self.benchmark,
            self.package,
            self.release,
            self.evidence,
            expected_commit=self.commit,
            expected_executable_sha256=self.exe_sha,
        )

    def test_real_production_manifest_and_parsers_bind_all_streams(self):
        result = self._verify()
        self.assertTrue(result["stream_coherence_verified"])
        self.assertEqual(result["alignment"]["measured_frame_interval"]["frame_count"], 4)
        self.assertEqual(result["candidate_commit"], self.commit)
        self.assertTrue(all(value is False for value in result["acceptance"].values()))

    def test_tampered_raw_stream_is_rejected_by_manifest_verifier(self):
        self.whole.write_text(self.whole.read_text(encoding="utf-8") + "6,16.000000\n", encoding="utf-8")
        with self.assertRaises(verifier.BenchmarkStreamCoherenceError):
            self._verify()

    def test_expected_candidate_identity_is_required(self):
        with self.assertRaises(verifier.BenchmarkStreamCoherenceError):
            verifier.verify_bound_stream_coherence(
                self.benchmark,
                self.package,
                self.release,
                self.evidence,
                expected_commit="b" * 40,
                expected_executable_sha256=self.exe_sha,
            )
        with self.assertRaises(verifier.BenchmarkStreamCoherenceError):
            verifier.verify_bound_stream_coherence(
                self.benchmark,
                self.package,
                self.release,
                self.evidence,
                expected_commit=self.commit,
                expected_executable_sha256="0" * 64,
            )

    def test_manifest_bound_interval_mismatch_is_rejected(self):
        self._write_phase(self.phase, count=3, warmup=2)
        benchmark = verifier.benchmark_manifest.build_benchmark_manifest(
            self.spec, self.package, self.release, self.evidence
        )
        with self.assertRaises(verifier.BenchmarkStreamCoherenceError):
            self._verify(benchmark)

    def test_local_native_provenance_rejects_fixture_memory_source(self):
        self.spec["provenance"] = {"evidence_class": "local_native", "machine_label": "fixture-host"}
        benchmark = verifier.benchmark_manifest.build_benchmark_manifest(
            self.spec, self.package, self.release, self.evidence
        )
        with self.assertRaises(verifier.BenchmarkStreamCoherenceError):
            self._verify(benchmark)


if __name__ == "__main__":
    unittest.main(verbosity=2)
