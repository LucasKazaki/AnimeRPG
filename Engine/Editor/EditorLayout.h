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

EditorLayout ComputeEditorLayout(int clientWidth, int clientHeight);
bool Contains(const EditorRect& outer, const EditorRect& inner);
bool Overlaps(const EditorRect& a, const EditorRect& b);

} // namespace Astral::Editor
