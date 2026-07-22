# AnimeRPG Agent Rules

This is an Unreal Engine 5 anime action RPG prototype. Work incrementally toward a necromancer vertical slice. Keep changes narrow and reviewable.

## Required behavior
- Read the task packet before editing.
- Modify only listed allowed files.
- Use C++/Blueprint conventions already present in the project; do not invent plugins.
- Report exact commands, files changed, tests/build results, risks, and follow-ups.

## Forbidden by default
No deletion, plugin installation, external API calls, networking changes, asset-wide refactors, credential access, or unrelated edits without Lucas's explicit approval.

## Review and merge
All implementation occurs in a dedicated worktree. Lucas approves merges. Codex reviews diffs without editing unless explicitly assigned.
