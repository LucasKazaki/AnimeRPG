# Creative starter content — author verification and remaining gates

Assessment: **portable pack/loader checks passed; independent and native Windows
acceptance pending. No UE5-quality/quantity parity, production-art, textured-runtime
or editor-integration acceptance.** This report is an author QA pass, not an
independent review or evidence of a running local Company Runtime.

## Scope and provenance

- Remote base inspected: PR #10 head `4b37959f5fbbccc387fc8ef54dbe753f66e27958`.
- Remote parent tree: `347aac83840dd5abdc5836a801de9263a6f4130b`.
- New additions only: starter archive/readme, creative plan, task, three Python
  scripts, one C++ probe and this report. No existing product, workflow, control,
  source-root CMake, game, editor, parent branch, main or scheduler change.
- Sandbox: isolated partial-source fixture with a dedicated git worktree at
  `/mnt/data/astral-creative/work`. Public clone/download transport did not succeed;
  **this was not a full repository build**. Existing sources were reconstructed
  from connected GitHub reads and checked byte-for-byte using Git blob hashes.
- No third-party artwork, model service, paid API, model download or dependency
  installation was used. All pack geometry and PNGs came from the checked-in
  procedural generator; compression used standard Python libraries.

Actual loader dependencies verified before compiling:

| Source | Git blob SHA |
|---|---|
| `Engine/Assets/StaticMesh.cpp` | `de26fbfa74e9cee14edb79ff2dd0d418b805fcc3` |
| `Engine/Assets/StaticMesh.h` | `67426f97df2ccbdfb18eca2788fc686ad6a052f6` |
| `Engine/Math/Math.h` | `7c743f2bb75497261335e1c7b35a597cf0e40e21` |

The source-only probe calls this actual loader; no mock parser stands in for it.
The separate Python validator checks stronger fixture invariants independently.
Reconstructed existing engine files and synthetic local fixture history are not
part of the publication.

## Executed checks

Environment: Linux; Python 3.13.5; GCC 14.2.0; Clang 17.0.0. All successful commands
below exited 0. Generation and archive verification outputs contain exactly
**39 files: 32 meshes, five PNGs, material recipes and manifest**. The manifest
lists 38 payload files rather than attempting a self-referential hash.

```
python Scripts/generate_starter_content.py --output ../generated-pack
python Scripts/generate_starter_content.py --output ../generated-pack --check
python Scripts/verify_starter_content.py ../generated-pack
python Scripts/test_starter_content.py

g++ -std=c++17 -Wall -Wextra -Werror -O0 -g -I. Engine/Assets/StaticMesh.cpp Tests/StarterContentProbe.cpp -o ../probe-debug
python Scripts/verify_starter_content.py ../generated-pack --probe ../probe-debug

g++ -std=c++17 -Wall -Wextra -Werror -O2 -DNDEBUG -I. Engine/Assets/StaticMesh.cpp Tests/StarterContentProbe.cpp -o ../probe-release
python Scripts/verify_starter_content.py ../generated-pack --probe ../probe-release

clang++ -std=c++17 -Wall -Wextra -Werror -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -I. Engine/Assets/StaticMesh.cpp Tests/StarterContentProbe.cpp -o ../probe-sanitized
ASAN_OPTIONS=detect_leaks=1 python Scripts/verify_starter_content.py ../generated-pack --probe ../probe-sanitized

python -m tarfile -e Content/Starter/astral-starter-v1.tar.xz ../archive-check
python Scripts/generate_starter_content.py --output ../archive-check --check
python Scripts/verify_starter_content.py ../archive-check
python Scripts/test_starter_content.py
python Scripts/verify_starter_content.py ../archive-check --probe ../probe-debug
python Scripts/verify_starter_content.py ../archive-check --probe ../probe-release
ASAN_OPTIONS=detect_leaks=1 python Scripts/verify_starter_content.py ../archive-check --probe ../probe-sanitized
python -m py_compile Scripts/generate_starter_content.py Scripts/verify_starter_content.py Scripts/test_starter_content.py
```

Results: 32/32 files loaded with expected vertex/edge counts in Debug, optimized
Release (explicit checks survive `NDEBUG`) and the sanitizer build. No sanitizer
diagnostic was observed. Both regression-suite runs passed **11 tests**; final
run took about 4.1 seconds. This is a bounded regression suite, not exhaustive
fuzzing, allocation-failure testing, load/unload stress or a soak.

Regression coverage: valid pack; exact deterministic regeneration; payload hash
corruption; out-of-range edge even with updated hash; NaN vertex even with updated
hash; broken PNG CRC even with updated hash; unexpected file; traversal path;
missing material reference; symlink rejection; existing-output refusal and
read-only exact checking. Tests that exercise refusal expect nonzero subprocess
status and verify existing bytes remain unchanged.

PNG checks cover signature, chunk length/order/CRC, bounded decompression, RGB8
format, dimensions and the generator's filter convention. All five files also
loaded through the separately available Pillow decoder. No Pillow dependency was
added to the repository's generator or test suite.

Author visual inspection: a contact sheet of all 32 wireframes showed recognizable
primitive, room-kit and prop silhouettes. The material atlas showed the intended
16 distinct calibration swatches. These were **offline diagnostic previews**, not
screenshots from Astral. They do not establish shaded topology, collisions,
animation, seamless PBR relief or professional art quality.

The final TAR.XZ archive uses fixed timestamps, sorted regular-file entries and
no links or executable assets. SHA-256:
`d9a72430e9da1ded0eca82783d135eb76971458fb3bc3834bf1465b2884ac7c5`.
Git blob SHA: `7c43fafe1c8fb885001181dcc8f7cc0baeeb5fe1`.
Python 3.13's tar CLI emitted its forward-looking extraction-filter deprecation
warning; extraction succeeded. Only the generated, hash-identified regular-file
archive was extracted, into a new owned sandbox path. Do not treat that command
as an untrusted-archive extraction service.

## Audit findings handled in the deliverable

- No invented GLB/FBX/VRM/PBR importer: source-only status is explicit.
- No falsely rigged mannequin: training dummy is static wireframe geometry.
- No inflated texture count: two atlases plus three diagnostics are five PNGs;
  16 surface recipes are not 16 complete shader systems.
- No naive atlas tiling claim: 64×64 tiles must be cropped before repeat/mipmap
  use, because there are no gutters.
- No implicit Unreal axis/scale convention: Y-up and pack-local metre convention
  are declared; future coordinate conversion still needs a real test.
- No copied Epic assets or implicit public license grant.
- No universal human-quality or UE5-parity claim; exact Unreal baseline inventory,
  blind art review and runtime comparisons remain pending.
- No unsafe generator overwrite: new directories only, or explicit read-only check.

## Required before acceptance/merge

Independent reviewer: inspect exact new-file diff, archive metadata, manifests,
source regeneration, negative tests, license boundaries and unsupported claims.
This author pass must not be relabeled independent QA.

Registered local executor: preserve the current project gates and resolve inherited
PR #6/#8/#10 acceptance. Run supported VS2022 external Debug/Release builds and
existing deterministic tests, compile/test the actual probe with MSVC, and retain
fresh native receipts. The probe is intentionally not registered in CTest in this
slice; no claim is made that hosted product CI executed the new tests.

Interactive/default-library acceptance remains blocked on an approved asset
catalog/importer, solid/textured rendering and scene/tutor workflow. Verify those
in separately admitted packets; do not modify the paused game's gameplay or clear
R0/soak gates just because these files exist. Lucas approves any merge.
