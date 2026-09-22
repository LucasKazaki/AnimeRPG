# CREATIVE-STARTER — planning and original engine fixtures

User-authorized scope, September 21, 2026 (America/New_York): audit creative
needs, research candidate tools, and add generic default textures/models.
This is not authorization to resume the paused RPG production roadmap.

Base: PR #10 head `4b37959f5fbbccc387fc8ef54dbe753f66e27958`, dependent on
unmerged PR #8 and #6. New draft branch only; no main, parent-branch, merge,
release, scheduler, local Company Runtime, R0, dependency or paid API changes.
Do not claim native/independent acceptance. Preserve all existing gates.

Allowed additions:
- `Content/Starter/` (original generic fixtures and instructions)
- `Scripts/generate_starter_content.py`
- `Scripts/verify_starter_content.py`
- `Scripts/test_starter_content.py`
- `Tests/StarterContentProbe.cpp`
- `Docs/Planning/CREATIVE-PRODUCTION-2026-09-21.md`
- `Docs/QA/CREATIVE-STARTER-2026-09-21.md`
- this task packet

Owned sandbox worktree: `/mnt/data/astral-creative/work`, isolated from the
partial-source fixture repository. This is NOT a full clone or a registered
Windows workspace. Exact fetched loader/math source bytes must match Git blob
hashes before they are used as a test fixture. Do not upload reconstructed engine
files or the fixture repository's synthetic history.

Commands (run from worktree; use a NEW output directory for generation):
```
python Scripts/generate_starter_content.py --output ../generated-pack
python Scripts/generate_starter_content.py --output ../generated-pack --check
python Scripts/verify_starter_content.py ../generated-pack
python Scripts/test_starter_content.py
g++ -std=c++17 -Wall -Wextra -Werror -O0 -g -I. Engine/Assets/StaticMesh.cpp Tests/StarterContentProbe.cpp -o ../probe-debug
python Scripts/verify_starter_content.py ../generated-pack --probe ../probe-debug
g++ -std=c++17 -Wall -Wextra -Werror -O2 -DNDEBUG -I. Engine/Assets/StaticMesh.cpp Tests/StarterContentProbe.cpp -o ../probe-release
python Scripts/verify_starter_content.py ../generated-pack --probe ../probe-release
```

Stop on deterministic validation failure, unknown output files, unsupported
write/approval requirements or native-platform access needs. Fix a captured
fixture defect in this allowlist before repeating, not through a renderer rewrite.
GitHub bytes must be verified against the tested candidate before publication.

Deferred native gate: independently review this diff and the inherited PR stack;
run the project's supported VS2022 external Debug/Release build and deterministic
CTest commands. The new probe is an explicit portable command, NOT registered
in product CTest in this slice. Compile it with MSVC C++17 and test all pack paths;
then obtain registered interactive Windows evidence. No editor installation,
texture rendering, scene loading, or tutorial GUI action is admitted or claimed.
