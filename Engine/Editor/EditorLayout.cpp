#include "Engine/Editor/EditorLayout.h"

#include <algorithm>
#include <cstdint>
#include <limits>

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
constexpr int kPanelHorizontalPadding = 8;
constexpr int kPanelLabelTop = 6;
constexpr int kPanelLabelHeight = 18;
constexpr int kPanelBodyTop = 28;
constexpr int kPanelBottomPadding = 8;
constexpr int kStatusHorizontalPadding = 8;
constexpr int kStatusVerticalPadding = 3;

std::int64_t Left(const EditorRect& rect) {
    return static_cast<std::int64_t>(rect.x);
}

std::int64_t Top(const EditorRect& rect) {
    return static_cast<std::int64_t>(rect.y);
}

std::int64_t Right(const EditorRect& rect) {
    return Left(rect) + static_cast<std::int64_t>(rect.width);
}

std::int64_t Bottom(const EditorRect& rect) {
    return Top(rect) + static_cast<std::int64_t>(rect.height);
}

int SaturatingAdd(int value, int offset) {
    const std::int64_t sum = static_cast<std::int64_t>(value)
        + static_cast<std::int64_t>(offset);
    const std::int64_t minimum = std::numeric_limits<int>::min();
    const std::int64_t maximum = std::numeric_limits<int>::max();
    return static_cast<int>(std::clamp(sum, minimum, maximum));
}
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
    const int buttonY = SaturatingAdd(toolbar.y, (toolbarHeight - buttonHeight) / 2);

    int buttonX = SaturatingAdd(toolbar.x, padding);
    auto nextButton = [&]() {
        const EditorRect rect{buttonX, buttonY, buttonWidth, buttonHeight};
        buttonX = SaturatingAdd(buttonX, buttonWidth + gap);
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

EditorPanelContentLayout ComputeEditorPanelContentLayout(const EditorRect& panel) {
    const int panelWidth = std::max(0, panel.width);
    const int panelHeight = std::max(0, panel.height);
    const int horizontalPadding = std::min(kPanelHorizontalPadding, panelWidth / 2);
    const int innerWidth = std::max(0, panelWidth - 2 * horizontalPadding);

    const int labelTop = std::min(kPanelLabelTop, panelHeight);
    const int labelHeight = std::min(kPanelLabelHeight, panelHeight - labelTop);
    const int bodyTop = std::min(kPanelBodyTop, panelHeight);
    const int bodyRemaining = panelHeight - bodyTop;
    const int bottomPadding = std::min(kPanelBottomPadding, bodyRemaining);

    EditorPanelContentLayout layout{};
    layout.label = {
        SaturatingAdd(panel.x, horizontalPadding),
        SaturatingAdd(panel.y, labelTop),
        innerWidth,
        labelHeight};
    layout.body = {
        SaturatingAdd(panel.x, horizontalPadding),
        SaturatingAdd(panel.y, bodyTop),
        innerWidth,
        std::max(0, bodyRemaining - bottomPadding)};
    return layout;
}

EditorRect ComputeEditorStatusContentLayout(const EditorRect& status) {
    const int statusWidth = std::max(0, status.width);
    const int statusHeight = std::max(0, status.height);
    const int horizontalPadding = std::min(kStatusHorizontalPadding, statusWidth / 2);
    const int verticalPadding = std::min(kStatusVerticalPadding, statusHeight / 2);
    return {
        SaturatingAdd(status.x, horizontalPadding),
        SaturatingAdd(status.y, verticalPadding),
        std::max(0, statusWidth - 2 * horizontalPadding),
        std::max(0, statusHeight - 2 * verticalPadding)};
}

EditorRect ComputeEditorViewportClipRect(const EditorRect& viewport) {
    return {
        viewport.x,
        viewport.y,
        std::max(0, viewport.width),
        std::max(0, viewport.height)};
}

bool Contains(const EditorRect& outer, const EditorRect& inner) {
    if (inner.width < 0 || inner.height < 0 || outer.width < 0 || outer.height < 0) {
        return false;
    }
    return Left(inner) >= Left(outer) && Top(inner) >= Top(outer)
        && Right(inner) <= Right(outer) && Bottom(inner) <= Bottom(outer);
}

bool Overlaps(const EditorRect& a, const EditorRect& b) {
    if (a.width <= 0 || a.height <= 0 || b.width <= 0 || b.height <= 0) {
        return false;
    }
    return Left(a) < Right(b) && Right(a) > Left(b)
        && Top(a) < Bottom(b) && Bottom(a) > Top(b);
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
