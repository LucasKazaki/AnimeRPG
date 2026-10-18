#!/usr/bin/env python3
"""Disposable filesystem/process controls; real engine packaging is a separate gate."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest import mock

sys.dont_write_bytecode = True
import package_engine_project as package
import release_manifest


# Verbatim excerpts of actual MSVC14.44 native-player reports retained at
# work/noon-package-first and work/noon-package-pe-format-first. Unrelated
# optional/section-header fields are omitted; DLL entries and section placement
# are unchanged. These are text fixtures, never a real package/build receipt.
RECORDED_PE_HEADERS = """FILE HEADER VALUES
            8664 machine (x64)
               7 number of sections

SECTION HEADER #2
  .rdata name
   5C9BE virtual size

SECTION HEADER #3
   .data name
   140D8 virtual size

  Summary

       15000 .data
        1000 .fptable
        D000 .pdata
       5D000 .rdata
        2000 .reloc
        1000 .rsrc
      13A000 .text
"""
RECORDED_DEPENDENCY_BLOCK = """  Image has the following dependencies:

    USER32.dll
    GDI32.dll
    WINMM.dll
    KERNEL32.dll
"""
RECORDED_PE_DEPENDENTS = """File Type: EXECUTABLE IMAGE

""" + RECORDED_DEPENDENCY_BLOCK + """
  Summary

       15000 .data
        1000 .fptable
        D000 .pdata
       5D000 .rdata
        2000 .reloc
        1000 .rsrc
      13A000 .text
"""
RECORDED_PE_COMBINED = RECORDED_PE_HEADERS.replace(
    "\nSECTION HEADER #3", "\n" + RECORDED_DEPENDENCY_BLOCK + "\nSECTION HEADER #3")
RECORDED_IMPORTS = ["GDI32.dll", "KERNEL32.dll", "USER32.dll", "WINMM.dll"]


class PackageControls(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="astral_engine_package_")
        self.root = Path(self.temp.name).resolve(strict=True)
        self.addCleanup(self.temp.cleanup)

    def content(self):
        content = self.root / "Content"
        content.mkdir()
        (content / "project.json").write_text('{"format":"astral-project","version":1}', encoding="utf-8")
        (content / "models").mkdir()
        (content / "models" / "original.bin").write_bytes(bytes(range(128)))
        (content / "empty-assets").mkdir()
        return content

    def args(self, content, destination=None):
        source = self.root / "source"
        source.mkdir(exist_ok=True)
        return argparse.Namespace(source=source, content=content, destination=destination or self.root / "package",
            build=self.root / "build", evidence=self.root / "evidence", cmake=self.root / "absent-cmake.exe",
            dumpbin=self.root / "absent-dumpbin.exe", jobs=2, notice=[])

    def test_copies_real_content_and_seals_bytes_with_existing_manifest(self):
        content = self.content()
        files = package.inventory(content)
        output = self.root / "copied"
        output.mkdir()
        for item in files:
            package.copy_checked(item, output / "Content" / item.relative)
        self.assertTrue((output / "Content" / "empty-assets").is_dir())
        self.assertEqual((output / "Content" / "models" / "original.bin").read_bytes(), bytes(range(128)))
        package.verify_content_snapshot(content, files)
        manifest = output / "release-manifest.json"
        data = release_manifest.write_release_manifest(output, manifest, "a"*40)
        receipt = release_manifest.verify_manifest(data, output, manifest_path=manifest, expected_commit="a"*40)
        self.assertTrue(receipt["manifest_integrity_verified"])
        (output / "Content" / "models" / "original.bin").write_bytes(b"changed")
        with self.assertRaises(release_manifest.ManifestError):
            release_manifest.verify_manifest(data, output, manifest_path=manifest)

    def test_changed_input_rejected_before_copy_and_existing_output_preserved(self):
        content = self.content()
        item = next(f for f in package.inventory(content) if f.relative == "project.json")
        item.source.write_text("changed", encoding="utf-8")
        target = self.root / "output.json"
        with self.assertRaises(package.PackageError):
            package.copy_checked(item, target)
        self.assertFalse(target.exists())
        fresh = package.input_file(item.source, item.relative)
        target.write_bytes(b"sentinel")
        with self.assertRaises(FileExistsError):
            package.copy_checked(fresh, target)
        self.assertEqual(target.read_bytes(), b"sentinel")

    def test_content_add_remove_and_empty_directory_changes_rejected(self):
        content = self.content()
        before = package.inventory(content)
        added = content / "new.txt"
        added.write_bytes(b"new")
        with self.assertRaises(package.PackageError):
            package.verify_content_snapshot(content, before)
        added.unlink()
        before = package.inventory(content)
        (content / "project.json").unlink()
        with self.assertRaises(package.PackageError):
            package.verify_content_snapshot(content, before)
        before = package.inventory(content)
        (content / "new-empty").mkdir()
        with self.assertRaises(package.PackageError):
            package.verify_content_snapshot(content, before)

    def test_byte_and_entry_budgets(self):
        content = self.content()
        with self.assertRaises(package.PackageError):
            package.inventory(content, max_bytes=8)
        with self.assertRaises(package.PackageError):
            package.inventory(content, max_files=1)

    def test_symlink_and_hard_link_do_not_copy_outside_bytes(self):
        content = self.content()
        outside = self.root / "outside.txt"
        outside.write_bytes(b"outside sentinel")
        link = content / "outside.txt"
        try:
            link.symlink_to(outside)
        except OSError as error:
            self.skipTest(f"file symlink unavailable: {error}")
        with self.assertRaises(package.PackageError):
            package.inventory(content)
        link.unlink()
        os.link(outside, link)
        with self.assertRaises(package.PackageError):
            package.inventory(content)
        self.assertEqual(outside.read_bytes(), b"outside sentinel")

    def test_directory_link_is_rejected_without_following_it(self):
        content = self.content()
        outside = self.root / "outside"
        outside.mkdir()
        (outside / "sentinel").write_bytes(b"keep")
        try:
            (content / "linked").symlink_to(outside, target_is_directory=True)
        except OSError as error:
            self.skipTest(f"directory symlink unavailable: {error}")
        with self.assertRaises(package.PackageError):
            package.inventory(content)
        self.assertEqual((outside / "sentinel").read_bytes(), b"keep")

    def test_windows_names_and_parent_escape_rejected_without_creating_them(self):
        invalid = ("../outside", "/absolute", "C:/drive", "a\\b", "a:stream", "con", "NUL.txt",
            "folder/LPT9.bin", "a.", "a ", "a//b", "a/./b", "a?b", "a\0b")
        for name in invalid:
            with self.subTest(name=name), self.assertRaises(package.PackageError):
                package.relative_name(name)
        self.assertEqual(str(package.relative_name("models/original.gltf")), "models/original.gltf")

    def test_existing_destination_is_preserved_before_any_tool_runs(self):
        content = self.content()
        args = self.args(content)
        args.destination.mkdir()
        sentinel = args.destination / "keep"
        sentinel.write_bytes(b"keep")
        with self.assertRaises(package.PackageError):
            package.stage(args)
        self.assertEqual(sentinel.read_bytes(), b"keep")
        self.assertFalse(args.evidence.exists())

    def test_project_entry_must_be_an_ordinary_file_before_tools(self):
        content = self.root / "Content"
        content.mkdir()
        args = self.args(content)
        for directory in (False, True):
            if directory:
                (content / "project.json").mkdir()
            with self.subTest(directory=directory), mock.patch.object(package, "source_binding", return_value={}), self.assertRaises(package.PackageError):
                package.stage(args)
            self.assertFalse(args.evidence.exists())
            self.assertFalse(args.destination.exists())

    def test_dot_and_superscript_device_names_rejected_as_strings(self):
        for name in (".", "COM\u00b9", "com\u00b2.txt", "folder/COM\u00b3.bin", "LPT\u00b9", "lpt\u00b2.txt", "folder/LPT\u00b3.bin"):
            with self.subTest(name=name), self.assertRaises(package.PackageError):
                package.relative_name(name)

    def test_broken_destination_link_is_preserved(self):
        content = self.content()
        args = self.args(content)
        try:
            args.destination.symlink_to(self.root / "missing", target_is_directory=True)
        except OSError as error:
            self.skipTest(f"directory symlink unavailable: {error}")
        with self.assertRaises(package.PackageError):
            package.stage(args)
        self.assertTrue(args.destination.is_symlink())
        self.assertFalse(args.evidence.exists())

    def test_evidence_and_build_overlap_fail_before_tools(self):
        content = self.content()
        for kind in ("source", "build", "content", "destination"):
            args = self.args(content)
            args.evidence = getattr(args, kind) / "evidence"
            with self.subTest(kind=kind), self.assertRaises(package.PackageError):
                package.stage(args)
            self.assertFalse(args.evidence.exists())

    def test_dumpbin_requires_actual_section_x64_and_exact_dependency_names(self):
        text = "FILE HEADER VALUES\n             8664 machine (x64)\n  Image has the following dependencies:\n\n    KERNEL32.dll\n    USER32.dll\n\n  Summary\n"
        self.assertEqual(package.inspect_imports(text), ["KERNEL32.dll", "USER32.dll"])
        for invalid in ("", " KERNEL32.dll\n", text.replace("8664 machine (x64)", "14C machine (x86)"),
                        text.replace("USER32.dll", "MSVCP140.dll"), text.replace("USER32.dll", "api-ms-win-unknown.dll"),
                        text.replace("    KERNEL32.dll", "").replace("    USER32.dll", "")):
            with self.subTest(text=invalid), self.assertRaises(package.PackageError):
                package.inspect_imports(invalid)

    def test_owned_timeout_records_failure_and_reaps_child(self):
        evidence = self.root / "timeout-evidence"
        evidence.mkdir()
        runner = package.Runner(evidence)
        with self.assertRaises(package.PackageError):
            runner.run("timeout", [sys.executable, "-c", "import time; time.sleep(30)"], cwd=self.root, timeout=0.1)
        steps = json.loads((evidence / "steps.json").read_text(encoding="utf-8"))
        self.assertTrue(steps[0]["timed_out"])
        self.assertIsNotNone(steps[0]["exit"])

    def test_dumpbin_rejects_unparsed_dependency_names(self):
        text = "  8664 machine (x64)\n  Image has the following dependencies:\n\n    KERNEL32.dll\n    custom runtime.dll\n\n  Summary\n"
        with self.assertRaises(package.PackageError):
            package.inspect_imports(text)

    def test_dumpbin_does_not_treat_a_dll_prefix_as_summary_heading(self):
        text = "  8664 machine (x64)\n  Image has the following dependencies:\n\n    KERNEL32.dll\n    Summary.dll\n\n  Summary\n"
        with self.assertRaises(package.PackageError):
            package.inspect_imports(text)

    def test_stage_accepts_recorded_pe_reports_only_with_separate_dumpbin_commands(self):
        # Orchestration unit only: dummy tools/binaries and captured text. Root's
        # separate real builds/dumpbin/launch gates remain required acceptance.
        content = self.content()
        original = {p.relative_to(content): p.read_bytes() for p in content.rglob('*') if p.is_file()}
        args = self.args(content)
        args.cmake.write_bytes(b"disposable dummy tool")
        args.dumpbin.write_bytes(b"disposable dummy tool")
        commands = []

        def replay(label, argv, **kwargs):
            self.assertEqual(kwargs['cwd'], args.source)
            if label == 'configure':
                (args.build / 'Release').mkdir(parents=True)
                return ''
            if label == 'build-release':
                for target in package.TARGETS:
                    (args.build / 'Release' / (target + '.exe')).write_bytes(b"dummy " + target.encode('ascii'))
                return ''
            self.assertEqual(Path(argv[0]), args.dumpbin)
            self.assertEqual(kwargs['timeout'], 30)
            commands.append(tuple(str(value) for value in argv))
            flags = tuple(str(value) for value in argv[1:-1])
            if flags == ('/headers', '/dependents'):
                return RECORDED_PE_COMBINED  # OLD fails on SECTION HEADER #3
            if flags == ('/headers',):
                return RECORDED_PE_HEADERS
            if flags == ('/dependents',):
                return RECORDED_PE_DEPENDENTS
            self.fail('Unexpected dumpbin command: ' + repr(argv))

        runner = mock.Mock()
        runner.run.side_effect = replay
        with mock.patch.object(package, 'source_binding', return_value={'commit': 'a'*40, 'tree': 'b'*40}), \
                mock.patch.object(package, 'admit_build'), mock.patch.object(package, 'Runner', return_value=runner):
            result = package.stage(args)
        self.assertEqual(result, args.destination)
        expected = []
        for target in package.TARGETS:
            executable = str(args.build / 'Release' / (target + '.exe'))
            expected.extend(((str(args.dumpbin), '/headers', executable),
                             (str(args.dumpbin), '/dependents', executable)))
        self.assertEqual(commands, expected)
        info = json.loads((result / 'BuildInfo.json').read_text(encoding='utf-8'))
        self.assertEqual(info['pe_imports'], {target: RECORDED_IMPORTS for target in package.TARGETS})
        self.assertTrue((args.evidence / 'package-result.json').exists())
        for relative, data in original.items():
            self.assertEqual((content / relative).read_bytes(), data)
            self.assertEqual((result / 'Content' / relative).read_bytes(), data)

    def test_actual_separate_dumpbin_reports_retain_complete_inventory(self):
        self.assertEqual(package.inspect_imports(RECORDED_PE_HEADERS + '\n' + RECORDED_PE_DEPENDENTS), RECORDED_IMPORTS)

    def test_separate_reports_reject_missing_or_unparsed_pe_proof(self):
        for headers, dependents in (
                (RECORDED_PE_HEADERS, RECORDED_PE_DEPENDENTS.replace('WINMM.dll', 'custom runtime.dll')),
                (RECORDED_PE_HEADERS, RECORDED_PE_DEPENDENTS.replace('WINMM.dll', 'MSVCP140.dll')),
                (RECORDED_PE_HEADERS, RECORDED_PE_DEPENDENTS.replace('WINMM.dll', 'Summary.dll')),
                (RECORDED_PE_HEADERS, RECORDED_PE_DEPENDENTS + RECORDED_PE_DEPENDENTS),
                ('', RECORDED_PE_DEPENDENTS),
                (RECORDED_PE_HEADERS.replace('8664 machine (x64)', '14C machine (x86)'), RECORDED_PE_DEPENDENTS),
                (RECORDED_PE_HEADERS, RECORDED_PE_DEPENDENTS.replace('  Summary', ''))):
            with self.subTest(headers=headers, dependents=dependents), self.assertRaises(package.PackageError):
                package.inspect_imports(headers + '\n' + dependents)


if __name__ == "__main__":
    unittest.main()
