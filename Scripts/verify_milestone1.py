#!/usr/bin/env python3
"""Static Milestone 1 gate used when a native compiler is unavailable."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
required = [
    ROOT / "CMakeLists.txt",
    ROOT / "Engine/Core/Clock.cpp",
    ROOT / "Engine/Core/Logger.cpp",
    ROOT / "Engine/Platform/Win32Application.cpp",
    ROOT / "Engine/Renderer/Renderer.cpp",
    ROOT / "Game/Main.cpp",
    ROOT / "Tests/MathTests.cpp",
]
markers = {
    ROOT / "Engine/Platform/Win32Application.cpp": ["PeekMessageW", "GetAsyncKeyState", "CreateWindowExW"],
    ROOT / "Engine/Renderer/Renderer.cpp": ["FillRect", "CreateSolidBrush"],
    ROOT / "Engine/Core/Clock.cpp": ["steady_clock", "Tick"],
    ROOT / "Engine/Core/Logger.cpp": ["file_", "flush"],
}

missing = [str(path.relative_to(ROOT)) for path in required if not path.is_file()]
if missing:
    raise SystemExit(f"missing required files: {missing}")
for path, expected in markers.items():
    text = path.read_text(encoding="utf-8")
    absent = [marker for marker in expected if marker not in text]
    if absent:
        raise SystemExit(f"{path.relative_to(ROOT)} missing markers: {absent}")
print(f"PASS: {len(required)} Milestone 1 source files and required system markers present")
