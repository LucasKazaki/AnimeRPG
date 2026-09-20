# E0 asset-loader repair evidence, 2026-09-20

Status: sandbox verification passed; hosted results belong to the PR run receipts;
local Windows/native acceptance and independent review remain pending.

## Provenance and scope

Remote base: `e12e6c559bf776ffc9c715c809a517f8e02ce5d5`, unmerged PR #6.
Sandbox used a dedicated worktree from a PARTIAL source fixture, not a full clone.
A public git clone failed because the container could not resolve github.com.
The connected GitHub tool provided the source. These reconstructed originals
matched their Git blob SHA before editing:

| Input | Git blob |
|---|---|
| StaticMesh.cpp | c87c9f7c35ec72f387002c8fdcc0bf004b6f2d71 |
| StaticMesh.h | 7b65164eb4e57ae4f814bd139a822b3f203a014f |
| Math.h, unchanged | 7c743f2bb75497261335e1c7b35a597cf0e40e21 |
| Top-level CMake before test registration | ccdac12aa214d52fc8395a49ac45a8d3850681cf |

Only the current text mesh loader is repaired. No change to gameplay, rendering,
engine choice, local scheduler, heartbeat, toolchain or production assets.

## Reproduction and results

A real optimized GCC build of the original loader accepted:

```text
vertex 0 0 0
ASTRAL_MESH 1
edge 0 0
```

The rejecting regression returned exit 1 before repair and exit 0 after repair.
The one-argument API remains and uses default limits. Failed loads still clear
both public arrays. Successful parsing commits validated local arrays together.
The existing format remains a whitespace-separated wireframe format, not glTF/FBX.

Executed in the sandbox with actual loader source:

| Gate | Result |
|---|---|
| GCC Debug configure/build/CTest | PASS, exit 0 |
| GCC optimized Release configure/build/CTest | PASS, exit 0 |
| Clang Debug ASan + UBSan, leak checking enabled | PASS, exit 0 |
| Re-run all three after final explicit standard-header change | PASS, exit 0 |
| Header-after-data regression against final loader | Rejected, exit 0 |
| Capability JSON source-reference/dependency-cycle validation | PASS |

Each CTest run executes 27 named groups in one executable. This includes 1,000
seeded mutation inputs, each loaded twice: 109 accepted and 891 rejected in this
sandbox. Every accepted result satisfies finite-coordinate/index/size invariants;
every failure leaves empty arrays; replayed results match. The suite also performs
200 valid/failure reload cycles plus a final recovery. These are bounded mutation
and invariant tests, not exhaustive fuzzing or a 24-hour soak.

Default file cap is 16 MiB; vertex/edge caps are 262144/524288. Trusted callers can
supply other limits. Exact, zero and exceeded limits are tested. Indices reject
signs and overflow; coordinates use classic-locale full-token parsing. Forward
edge references and whitespace remain supported. The limits are implementation
safety defaults, not approved production-scene budgets or UE parity evidence.

## Reproduction commands

From a correctly owned checkout, use distinct external build directories. Run
one command at a time and stop on a nonzero exit:

```sh
cmake -S Tests/AssetValidation -B ../asset-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build ../asset-debug --config Debug
ctest --test-dir ../asset-debug -C Debug --output-on-failure --no-tests=error -V
cmake -S Tests/AssetValidation -B ../asset-release -DCMAKE_BUILD_TYPE=Release
cmake --build ../asset-release --config Release
ctest --test-dir ../asset-release -C Release --output-on-failure --no-tests=error -V
cmake -S Tests/AssetValidation -B ../asset-sanitize -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=clang++ -DASTRAL_ASSET_SANITIZERS=ON
cmake --build ../asset-sanitize
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 ctest --test-dir ../asset-sanitize --output-on-failure --no-tests=error -V
```

The sanitizer command is a Unix-host configuration, not an instruction to install
Clang or use those environment-assignment lines in PowerShell. Local multi-config
Windows builds must set both build and CTest configuration explicitly.
The new top-level native test is registered through `astral_add_test`; that makes
15 native targets, nine domain and six GUI. Historical 14-target counts predate
this repair. The portable subproject is intentionally only the asset loader.

## Local executor handoff and remaining gates

The existing Company Runtime must admit the native job in one owned worktree.
Read this task and PR #6/#7, inspect the exact candidate SHA and any existing local
changes, then build the full application with Visual Studio 2022 in an external
directory. Run Debug/Release domain tests and the new asset tests, then run all
GUI smokes on one exclusively owned interactive desktop. Keep the R0 runner off.
Local models must attach commands, toolchain/machine versions, UTC timestamps,
stdout/stderr, exit codes and artifact hashes. An author's self-review is not
independent acceptance.

Further tests still needed: Windows path/permissions behavior, read failures,
injected allocation failure, actively modified files, real integration usage and
performance, native GUI, package launch, and the existing engine stress/soak gates.
The loader assumes ordinary regular files not concurrently replaced by hostile
filesystem writers. It is not a filesystem sandbox or wall-clock I/O timeout.
Finite coordinate validation does not prove every downstream math operation is
safe for arbitrarily large coordinates. No GPU, local PC, UE editor or Unity editor
was run in this sandbox. No parity or engine-completion claim follows from this fix.
