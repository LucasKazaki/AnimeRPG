# GAME-HOURLY-SETUP-2026-09-22

Owner: `animerpg-game-hourly`. Phase: documentation and research setup only.
Research baseline: `771b61ac116dfa4a70d81b53a390f672aaf3bf4e`.
Integration baseline after concurrent engine merges: `d4bb31f2702ebec84d30ff2eab29fcf0bd527301`.
PR #6 integration was first observed at `8df6d2fc3814564c39049bb154361205f0f00309`.
Authority: Lucas's September 22 request for a separate hourly game worker and explicit follow-up to push/merge this cycle into main.

## Allowed paths

- `AGENTS.md`: append scoped operator authorization, preserve the newly integrated PR #6 rules verbatim.
- `GAME_DEVELOPMENT_CONTROL.md`: append the same dated operator update, preserving engine acceptance requirements and eliminating the otherwise contradictory blanket pause.
- `Docs/Agents/ANIMERPG-HOURLY.md`: operating/research/merge contract.
- `Docs/Agents/animerpg-hourly/BACKLOG.json`: five reference candidates plus one separate sourced community request.
- `Docs/Agents/animerpg-hourly/validate_setup.py`: read-only, standard-library setup validator requested by independent review.
- `Tasks/GAME-HOURLY-SETUP-2026-09-22.md`: this packet.

No Engine/Game/Tests source, CMake, CI, runtime databases, other task records, art archives, dependencies, or other worker branches may be changed in this packet. It does not admit an implementation packet or satisfy gameplay acceptance. The engine worker's own scope is unchanged.

The additional control-file path was admitted during setup because PR #6 reached main concurrently at 04:37 UTC. Preserve that commit and its native-test safety rules; reconcile through a non-forced merge commit on this worker branch. Do not replace main with the old setup tree. The operating contract and backlog retain their original research baseline as historical evidence.

## Verification and merge conditions

Validate UTF-8/JSON, unique source/feature IDs, five-plus-one coverage, valid source references, dependency IDs, nonempty acceptance criteria, zero fabricated implementation/test/review/merge receipts, and exact allowed paths. Review operator wording for narrow authority and retained testing/independent-review gates. Check the final GitHub diff against the observed baseline and read back published blob hashes.

Use a dedicated branch based on the live main revision; reconcile if main moved. Open a documentation-only PR to main, inspect applicable checks and merge with expected-head-SHA protection after validation. This is implementation of the operator's operating instructions, not independent approval of game code. If a required check is unavailable or fails, retain the PR and report the exact limitation. Do not change repository protections or merge unrelated engine work.

The setup sandbox could not clone GitHub because DNS resolution failed. Connected GitHub file reads were available. Structural validation of authored operating files is not a full repository build. No product compiler, native Windows/GPU test, performance measurement, or independent gameplay review is claimed.

Stop after verified documentation merge or a real merge/check blocker. Put actual validation commands/results, head/merge SHA and final readback in the PR/task receipt; do not invent a self-referential merge hash inside this commit. Rollback is a scoped revert of this setup through a PR, preserving unrelated subsequent changes.

## Reproduce the setup checks

Use a clean checkout of this PR's **final setup head**, not an arbitrary later
game-development revision. The zero-implementation assertions deliberately
validate this historical setup, not future feature progress. No network, write,
reset, engine build or local-runtime action is performed by the validator.
From that checkout, run these commands in PowerShell or a POSIX shell:

```text
python Docs/Agents/animerpg-hourly/validate_setup.py --scope-base d4bb31f2702ebec84d30ff2eab29fcf0bd527301 --scope-head HEAD
git diff --check d4bb31f2702ebec84d30ff2eab29fcf0bd527301...HEAD
```

Expected: 13 setup checks pass, validator exits 0, and diff check exits 0.
The validator resolves explicit commits, requires the base to be an ancestor,
checks the exact six-path allowlist/no deletion, compares LF-normalized authored file bytes
to Git blobs (allowing normal Windows CRLF checkouts) at the supplied head, verifies preserved pre-update control-file
hashes, and checks the source/feature/dependency/acceptance/authority records.
It does not fetch source URLs, prove gameplay behavior, or replace independent
review/native acceptance. Every Git subprocess has a 20-second timeout.
Missing files/commits or malformed JSON fail rather than producing a pass.
Record the actual final candidate SHA in the PR receipt; the commit cannot
contain its own SHA. For a sandbox partial-source fixture, supply its recorded
synthetic base/head instead and label that evidence separately from repo CI.

## Continuation

The hourly task is configured separately as **AnimeRPG Game Development**. Its durable entry point is `Docs/Agents/ANIMERPG-HOURLY.md`. After setup, resolve live game/engine ownership and admit a bounded GAME-001 packet; do not replay this setup every hour. Report six researched items and zero new implemented gameplay features for this setup.
