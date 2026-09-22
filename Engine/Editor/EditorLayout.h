#pragma once

namespace Astral::Editor {

struct EditorRect {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
};

struct EditorLayout {
    EditorRect toolbar;
    EditorRect outliner;
    EditorRect viewport;
    EditorRect inspector;
    EditorRect assets;
    EditorRect status;
};

struct EditorToolbarLayout {
    EditorRect select;
    EditorRect move;
    EditorRect rotate;
    EditorRect scale;
    EditorRect play;
};

struct EditorPanelContentLayout {
    EditorRect label;
    EditorRect body;
};

enum class EditorTool {
    Select,
    Move,
    Rotate,
    Scale,
    Play,
};

enum class EditorMessageLoopAction {
    Dispatch,
    Quit,
    Error,
};

EditorLayout ComputeEditorLayout(int clientWidth, int clientHeight);
EditorToolbarLayout ComputeEditorToolbarLayout(const EditorRect& toolbar);
EditorPanelContentLayout ComputeEditorPanelContentLayout(const EditorRect& panel);
EditorRect ComputeEditorStatusContentLayout(const EditorRect& status);
EditorRect ComputeEditorViewportClipRect(const EditorRect& viewport);
bool Contains(const EditorRect& outer, const EditorRect& inner);
bool Overlaps(const EditorRect& a, const EditorRect& b);
bool IsEditorToolAvailable(EditorTool tool);
EditorMessageLoopAction ClassifyEditorMessageResult(int result);

} // namespace Astral::Editor
