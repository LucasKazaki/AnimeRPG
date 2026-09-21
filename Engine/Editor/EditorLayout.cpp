#include "Engine/Editor/EditorLayout.h"

#include <algorithm>

namespace Astral::Editor {
namespace {
constexpr int kToolbarHeight = 42;
constexpr int kStatusHeight = 24;
constexpr int kPreferredSideWidth = 240;
constexpr int kPreferredAssetHeight = 180;
}

EditorLayout ComputeEditorLayout(int clientWidth, int clientHeight) {
    clientWidth = std::max(0, clientWidth);
    clientHeight = std::max(0, clientHeight);

    const int toolbarHeight = std::min(kToolbarHeight, clientHeight);
    const int remainingAfterToolbar = clientHeight - toolbarHeight;
    const int statusHeight = std::min(kStatusHeight, remainingAfterToolbar);
    const int contentHeight = remainingAfterToolbar - statusHeight;

    const int sideWidthCap = clientWidth / 3;
    const int leftWidth = std::min(kPreferredSideWidth, sideWidthCap);
    const int rightWidth = std::min(kPreferredSideWidth, sideWidthCap);
    const int centerWidth = std::max(0, clientWidth - leftWidth - rightWidth);
    const int assetHeight = std::min(kPreferredAssetHeight, contentHeight / 3);
    const int viewportHeight = std::max(0, contentHeight - assetHeight);

    EditorLayout layout{};
    layout.toolbar = {0, 0, clientWidth, toolbarHeight};
    layout.outliner = {0, toolbarHeight, leftWidth, contentHeight};
    layout.viewport = {leftWidth, toolbarHeight, centerWidth, viewportHeight};
    layout.inspector = {leftWidth + centerWidth, toolbarHeight, rightWidth, contentHeight};
    layout.assets = {leftWidth, toolbarHeight + viewportHeight, centerWidth, assetHeight};
    layout.status = {0, toolbarHeight + contentHeight, clientWidth, statusHeight};
    return layout;
}

bool Contains(const EditorRect& outer, const EditorRect& inner) {
    if (inner.width < 0 || inner.height < 0 || outer.width < 0 || outer.height < 0) {
        return false;
    }
    return inner.x >= outer.x && inner.y >= outer.y
        && inner.x + inner.width <= outer.x + outer.width
        && inner.y + inner.height <= outer.y + outer.height;
}

bool Overlaps(const EditorRect& a, const EditorRect& b) {
    if (a.width <= 0 || a.height <= 0 || b.width <= 0 || b.height <= 0) {
        return false;
    }
    return a.x < b.x + b.width && a.x + a.width > b.x
        && a.y < b.y + b.height && a.y + a.height > b.y;
}

} // namespace Astral::Editor
