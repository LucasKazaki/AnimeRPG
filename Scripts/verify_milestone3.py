from pathlib import Path

CHECKS = {
    "Engine/Scene/PlayerController.h": [
        "struct MovementInput",
        "struct MovementBounds",
        "class PlayerController",
    ],
    "Engine/Scene/PlayerController.cpp": [
        "PlayerController::Update",
        "movementSpeed_",
        "std::clamp",
    ],
    "Engine/Scene/Camera.cpp": ["OrthographicCamera::Follow"],
    "Engine/Platform/Win32Application.cpp": [
        "Clock clock",
        "clock.Tick",
        "const auto keyDown =",
        "benchmarkRunControl.SuppressLiveInput()",
        "GetAsyncKeyState(virtualKey)",
        "keyDown('W')",
        "camera_.Follow",
        "WM_CLOSE",
    ],
    "Tests/SceneTests.cpp": ["PlayerController", "0.5f", "Follow"],
}


def main() -> int:
    missing = []
    for relative_path, markers in CHECKS.items():
        text = Path(relative_path).read_text(encoding="utf-8")
        for marker in markers:
            if marker not in text:
                missing.append(f"{relative_path}: missing {marker!r}")
    if missing:
        print("M3 static verification: FAIL")
        print("\n".join(missing))
        return 1
    print("M3 static verification: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
