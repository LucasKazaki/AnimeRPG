#!/usr/bin/env python3
"""Static Milestone 2 gate; native build/runtime checks remain separate."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
required = [
    ROOT / "Engine/Assets/StaticMesh.cpp",
    ROOT / "Engine/Scene/Transform.cpp",
    ROOT / "Engine/Scene/Camera.cpp",
    ROOT / "Engine/Renderer/Renderer.cpp",
    ROOT / "Game/Assets/debug_triangle.mesh",
    ROOT / "Tests/SceneTests.cpp",
]
markers = {
    ROOT / "Engine/Assets/StaticMesh.cpp": ["LoadFromFile", "ASTRAL_MESH", "vertices_.clear"],
    # RA-001 replaces recursive parent lookup with checked iterative traversal.
    # Presence only: SceneTests/NumericAuditTests verify hierarchy behavior.
    ROOT / "Engine/Scene/Transform.cpp": ["TryWorldPosition", "node = node->parent"],
    ROOT / "Engine/Scene/Camera.cpp": ["WorldToScreen", "worldWidth", "worldHeight"],
    # M5 evolved the original RenderDebugScene entry point into RenderWorld.
    ROOT / "Engine/Renderer/Renderer.cpp": ["RenderWorld", "CreatePen", "LineTo"],
    ROOT / "Engine/Platform/Win32Application.cpp": ["g_renderer.RenderWorld", "WM_PAINT"],
}

missing = [str(path.relative_to(ROOT)) for path in required if not path.is_file()]
if missing:
    raise SystemExit(f"missing required files: {missing}")
for path, expected in markers.items():
    text = path.read_text(encoding="utf-8")
    absent = [marker for marker in expected if marker not in text]
    if absent:
        raise SystemExit(f"{path.relative_to(ROOT)} missing markers: {absent}")
mesh = (ROOT / "Game/Assets/debug_triangle.mesh").read_text(encoding="utf-8").splitlines()
if mesh[0] != "ASTRAL_MESH 1" or sum(line.startswith("vertex ") for line in mesh) != 3:
    raise SystemExit("debug mesh fixture does not contain the expected header and three vertices")
print(f"PASS: {len(required)} M2 source/fixture files and required implementation markers present")
