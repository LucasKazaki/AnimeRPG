#include "Engine/Editor/EditorLayout.h"

#include <algorithm>

namespace Astral::Editor {
namespace {
constexpr int kToolbarHeight = 42;
constexpr int kStatusHeight = 24;
constexpr int kPreferredSideWidth = 240;
constexpr int kPreferredAssetHeight = 180;
constexpr int kToolbarHorizontalPadding = 8;
constexpr int kToolbarButtonGap = 6;
constexpr int kToolbarButtonPreferredWidth = 104;
constexpr int kToolbarButtonPreferredHeight = 28;
constexpr int kToolbarButtonCount = 5;
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

EditorToolbarLayout ComputeEditorToolbarLayout(const EditorRect& toolbar) {
    const int toolbarWidth = std::max(0, toolbar.width);
    const int toolbarHeight = std::max(0, toolbar.height);
    const int padding = std::min(kToolbarHorizontalPadding, toolbarWidth / 2);
    const int availableWidth = std::max(0, toolbarWidth - 2 * padding);

    int gap = 0;
    if (availableWidth > kToolbarButtonCount) {
        gap = std::min(kToolbarButtonGap,
            (availableWidth - kToolbarButtonCount) / (kToolbarButtonCount - 1));
    }
    const int totalGap = gap * (kToolbarButtonCount - 1);
    const int buttonWidth = std::min(kToolbarButtonPreferredWidth,
        std::max(0, (availableWidth - totalGap) / kToolbarButtonCount));
    const int buttonHeight = std::min(kToolbarButtonPreferredHeight, toolbarHeight);
    const int buttonY = toolbar.y + (toolbarHeight - buttonHeight) / 2;

    int buttonX = toolbar.x + padding;
    auto nextButton = [&]() {
        const EditorRect rect{buttonX, buttonY, buttonWidth, buttonHeight};
        buttonX += buttonWidth + gap;
        return rect;
    };

    EditorToolbarLayout layout{};
    layout.select = nextButton();
    layout.move = nextButton();
    layout.rotate = nextButton();
    layout.scale = nextButton();
    layout.play = nextButton();
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

bool IsEditorToolAvailable(EditorTool tool) {
    switch (tool) {
    case EditorTool::Select:
    case EditorTool::Move:
    case EditorTool::Rotate:
    case EditorTool::Scale:
    case EditorTool::Play:
        return false;
    }
    return false;
}

} // namespace Astral::Editor
