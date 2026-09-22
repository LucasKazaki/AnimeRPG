# R0 Windows dependency inventory evidence, 2026-09-20

Status: portable parser tests passed in the sandbox. Hosted Windows inspection of a
real Release `AstralGame.exe` is pending publication of the candidate. R0 itself was
not invoked, no redistributable was downloaded/copied, and clean-machine acceptance
is not claimed.

## Sandbox scope

The sandbox contains the new standalone PE inspector and its tests only. It is not
Lucas's Windows host and does not contain a native AstralGame executable. The parser
uses only the Python standard library and has a 512 MiB input-size ceiling plus
bounded section/import-descriptor and DLL-name parsing.

Executed commands:

```text
python -m py_compile Scripts/inspect_pe_dependencies.py Scripts/test_pe_dependencies.py
python Scripts/test_pe_dependencies.py
```

Portable result: **5 test methods discovered, 4 passed, 1 Windows-only real-PE test
skipped, exit 0**. Covered behavior includes PE32+ import and delay-import parsing,
Debug UCRT rejection through the CLI, Release MSVC runtime inventory without a false
clean-machine claim, malformed/non-PE rejection, JSON report generation, and bounded
replay through subprocess execution.

Sandbox SHA-256 before GitHub publication:

- `Scripts/inspect_pe_dependencies.py`:
  `498cb73f834dfda5d022e29136201890ec864aed3eab748fb65b01e2221aa871`
- `Scripts/test_pe_dependencies.py`:
  `0d684584cd4d182e27f4156bab503c743db11a76c5b76f7644172962915519d5`

## Hosted Windows acceptance to collect

The PR's existing `windows-2022` job must run the parser tests. On Windows, the fifth
method must inspect the real `sys.executable`, confirming the parser works against a
real PE rather than only synthetic fixtures. After the normal Release build, CI must
run:

```text
python Scripts/inspect_pe_dependencies.py <BUILD_ROOT>/Release/AstralGame.exe \
  --json <RUNNER_TEMP>/AstralGame-runtime-dependencies.json \
  --fail-on-debug-runtime
```

The JSON printed into the CI log must identify the real executable hash, architecture,
normal/delay imports, known MSVC/UCRT imports, and whether VC runtime deployment review
is required. A Debug CRT import is a hard failure. A normal Release VC runtime import
is not itself a failure because Microsoft supports redistributable deployment; it
keeps the separate clean-machine prerequisite decision open.

## Remaining gates

This packet does not prove that a target Windows machine has the imported libraries,
does not discover DLLs loaded only by arbitrary runtime `LoadLibrary` calls, and does
not validate install/uninstall behavior. The registered local executor still needs
the exact candidate SHA, native interactive RuntimeSmoke/package launch, dependency
availability on a supported clean machine or an approved prerequisite strategy,
artifact hashes, and an independent review. PR #9 and issue #7 must remain open until
those gates are recorded.
