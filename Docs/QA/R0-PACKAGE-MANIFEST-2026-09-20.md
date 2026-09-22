# R0 package manifest integrity evidence, 2026-09-20

Status before publication: sandbox contracts passed. Hosted Windows receipt must be appended to PR #9 only after the exact published candidate succeeds.

## Candidate scope

Baseline: PR #9 head `8ed1cb6996b794a797ddbe9bbce52c6f18d60c03`.

This packet adds package byte/provenance verification. It does not invoke R0, change engine/game code, install a Redistributable, launch the packaged GUI, prove a clean Windows machine, or provide independent acceptance.

## Sandbox evidence

Executed against the exact proposed `Scripts/release_manifest.py` and `Scripts/test_release_manifest.py` in a disposable Linux directory:

- `python -m py_compile release_manifest.py test_release_manifest.py`: PASS, exit 0.
- `python test_release_manifest.py`: **7/7 PASS**, exit 0.

Covered contracts include successful create/verify round-trip, manifest self-exclusion, explicit commit and executable-SHA binding, changed bytes, missing files, unmanifested files, path traversal/absolute/drive/backslash rejection, Windows case-collision rejection, schema-v2 aggregate tampering, historical v1 verification, refusal to trust the recorded package root, symlink escape rejection where supported, CLI JSON output, and nonzero exit after tampering.

## Claim boundary

A passing report means the listed package bytes under the explicitly supplied root match the manifest at verification time. It does not establish executable launch, runtime prerequisite installation, target-machine compatibility, signing trust, malware safety, performance, soak stability, or independent review. Package-directory mutation concurrent with verification is outside this bounded tool's acceptance model and must not occur during a release check.

## Hosted/local handoff

The dedicated Windows workflow builds the real Release `AstralGame.exe`, stages a disposable hosted package, records the actual checkout commit and executable SHA-256, creates the schema-v2 manifest, verifies it against those exact values, checks claim boundaries, and confirms the checkout remains clean. Record the workflow run/job plus exact package hash in PR #9 after success.

When R0 is eventually authorized locally, run the verifier against the finished R0 package using the admitted revision and independently recorded `AstralGame.exe` hash. Retain the manifest and verification JSON alongside command evidence. This still does not replace interactive package launch on a supported clean Windows machine or independent review.
