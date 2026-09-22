#include "Engine/Editor/EditorLayout.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace {
using Astral::Editor::ComputeEditorLayout;
using Astral::Editor::ComputeEditorPanelContentLayout;
using Astral::Editor::ComputeEditorStatusContentLayout;
using Astral::Editor::ComputeEditorToolbarLayout;
using Astral::Editor::ComputeEditorViewportClipRect;
using Astral::Editor::Contains;
using Astral::Editor::EditorRect;
using Astral::Editor::EditorTool;
using Astral::Editor::IsEditorToolAvailable;
using Astral::Editor::Overlaps;

bool Expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        return false;
    }
    return true;
}

bool CheckToolbarLayout(const EditorRect& toolbar) {
    const auto layout = ComputeEditorToolbarLayout(toolbar);
    const std::array<EditorRect, 5> buttons{
        layout.select, layout.move, layout.rotate, layout.scale, layout.play};
    bool ok = true;
    for (const auto& button : buttons) {
        ok &= Expect(Contains(toolbar, button), "toolbar button outside toolbar");
    }
    for (std::size_t left = 0; left < buttons.size(); ++left) {
        for (std::size_t right = left + 1; right < buttons.size(); ++right) {
            ok &= Expect(!Overlaps(buttons[left], buttons[right]),
                "toolbar buttons overlap");
        }
    }
    return ok;
}

bool CheckPanelContentLayout(const EditorRect& panel) {
    const auto content = ComputeEditorPanelContentLayout(panel);
    bool ok = true;
    ok &= Expect(Contains(panel, content.label), "panel label outside panel");
    ok &= Expect(Contains(panel, content.body), "panel body outside panel");
    ok &= Expect(!Overlaps(content.label, content.body), "panel label overlaps body");
    return ok;
}

bool CheckLayout(int width, int height) {
    const auto layout = ComputeEditorLayout(width, height);
    const EditorRect client{0, 0, width < 0 ? 0 : width, height < 0 ? 0 : height};
    bool ok = true;
    ok &= Expect(Contains(client, layout.toolbar), "toolbar outside client");
    ok &= Expect(Contains(client, layout.outliner), "outliner outside client");
    ok &= Expect(Contains(client, layout.viewport), "viewport outside client");
    ok &= Expect(Contains(client, layout.inspector), "inspector outside client");
    ok &= Expect(Contains(client, layout.assets), "assets outside client");
    ok &= Expect(Contains(client, layout.status), "status outside client");
    ok &= Expect(!Overlaps(layout.outliner, layout.viewport), "outliner overlaps viewport");
    ok &= Expect(!Overlaps(layout.viewport, layout.inspector), "viewport overlaps inspector");
    ok &= Expect(!Overlaps(layout.viewport, layout.assets), "viewport overlaps asset browser");
    ok &= Expect(!Overlaps(layout.toolbar, layout.outliner), "toolbar overlaps content");
    ok &= Expect(!Overlaps(layout.status, layout.assets), "status overlaps assets");
    ok &= CheckToolbarLayout(layout.toolbar);
    ok &= CheckPanelContentLayout(layout.outliner);
    ok &= CheckPanelContentLayout(layout.inspector);
    ok &= CheckPanelContentLayout(layout.assets);
    ok &= Expect(Contains(layout.status, ComputeEditorStatusContentLayout(layout.status)),
        "status text control outside status area");
    ok &= Expect(Contains(layout.viewport, ComputeEditorViewportClipRect(layout.viewport)),
        "viewport clip outside viewport");
    return ok;
}
}

int main() {
    bool ok = true;
    ok &= CheckLayout(1280, 720);
    ok &= CheckLayout(1920, 1080);
    ok &= CheckLayout(800, 600);
    ok &= CheckLayout(320, 200);
    ok &= CheckLayout(100, 100);
    ok &= CheckLayout(100, 50);
    ok &= CheckLayout(16, 16);
    ok &= CheckLayout(1, 1);
    ok &= CheckLayout(0, 0);
    ok &= CheckLayout(-1, -1);

    const auto standard = ComputeEditorLayout(1280, 720);
    ok &= Expect(standard.toolbar.height == 42, "standard toolbar height changed");
    ok &= Expect(standard.status.height == 24, "standard status height changed");
    ok &= Expect(standard.outliner.width == 240, "standard outliner width changed");
    ok &= Expect(standard.inspector.width == 240, "standard inspector width changed");
    ok &= Expect(standard.assets.height == 180, "standard asset browser height changed");
    ok &= Expect(standard.viewport.width == 800, "standard viewport width changed");
    ok &= Expect(standard.viewport.height == 474, "standard viewport height changed");

    const auto standardToolbar = ComputeEditorToolbarLayout(standard.toolbar);
    ok &= Expect(standardToolbar.select.width == 104,
        "standard toolbar button width changed");
    ok &= Expect(standardToolbar.select.height == 28,
        "standard toolbar button height changed");
    ok &= Expect(standardToolbar.play.x + standardToolbar.play.width <= 1280,
        "standard toolbar overflows client width");

    const auto standardOutlinerContent = ComputeEditorPanelContentLayout(standard.outliner);
    ok &= Expect(standardOutlinerContent.label.x == standard.outliner.x + 8,
        "standard panel horizontal padding changed");
    ok &= Expect(standardOutlinerContent.label.y == standard.outliner.y + 6,
        "standard panel label top changed");
    ok &= Expect(standardOutlinerContent.label.height == 18,
        "standard panel label height changed");
    ok &= Expect(standardOutlinerContent.body.y == standard.outliner.y + 28,
        "standard panel body top changed");
    ok &= Expect(standardOutlinerContent.body.height == standard.outliner.height - 36,
        "standard panel body height changed");

    const auto standardStatusContent = ComputeEditorStatusContentLayout(standard.status);
    ok &= Expect(standardStatusContent.x == standard.status.x + 8,
        "standard status horizontal padding changed");
    ok &= Expect(standardStatusContent.y == standard.status.y + 3,
        "standard status vertical padding changed");
    ok &= Expect(standardStatusContent.height == standard.status.height - 6,
        "standard status height changed");

    const auto standardViewportClip = ComputeEditorViewportClipRect(standard.viewport);
    ok &= Expect(standardViewportClip.x == standard.viewport.x
            && standardViewportClip.y == standard.viewport.y
            && standardViewportClip.width == standard.viewport.width
            && standardViewportClip.height == standard.viewport.height,
        "standard viewport clip must match viewport bounds");

    const auto malformedViewportClip = ComputeEditorViewportClipRect({12, 34, -50, -60});
    ok &= Expect(malformedViewportClip.x == 12 && malformedViewportClip.y == 34,
        "viewport clip origin changed for malformed dimensions");
    ok &= Expect(malformedViewportClip.width == 0 && malformedViewportClip.height == 0,
        "viewport clip must clamp malformed dimensions to zero");

    const auto narrow = ComputeEditorLayout(320, 200);
    const auto narrowToolbar = ComputeEditorToolbarLayout(narrow.toolbar);
    ok &= Expect(narrowToolbar.select.width == 56,
        "narrow toolbar should compress buttons instead of overflowing");
    ok &= Expect(narrowToolbar.play.x + narrowToolbar.play.width <= 320,
        "narrow toolbar still overflows client width");

    const auto shortLayout = ComputeEditorLayout(100, 50);
    const auto shortAssetsContent = ComputeEditorPanelContentLayout(shortLayout.assets);
    ok &= Expect(shortAssetsContent.label.height == 0,
        "zero-height asset panel should collapse its label");
    ok &= Expect(shortAssetsContent.body.height == 0,
        "zero-height asset panel should collapse its body");

    const int maxInt = std::numeric_limits<int>::max();
    const int minInt = std::numeric_limits<int>::min();
    const EditorRect highOuter{maxInt - 4, maxInt - 4, 10, 10};
    const EditorRect highInner{maxInt - 3, maxInt - 3, 2, 2};
    const EditorRect highOutside{maxInt, maxInt, 10, 10};
    ok &= Expect(Contains(highOuter, highInner),
        "containment must remain defined when rectangle edges exceed int range");
    ok &= Expect(!Contains(highOuter, highOutside),
        "overflow-safe containment must reject an inner rectangle extending beyond outer bounds");
    ok &= Expect(Overlaps(highOuter, highInner),
        "overlap must remain defined when rectangle edges exceed int range");

    const EditorRect lowOuter{minInt, minInt, 10, 10};
    const EditorRect lowInner{minInt + 1, minInt + 1, 2, 2};
    ok &= Expect(Contains(lowOuter, lowInner),
        "containment must remain defined near minimum int coordinates");
    ok &= Expect(Overlaps(lowOuter, lowInner),
        "overlap must remain defined near minimum int coordinates");

    const EditorRect touchingLeft{maxInt - 10, 0, 5, 5};
    const EditorRect touchingRight{maxInt - 5, 0, 5, 5};
    ok &= Expect(!Overlaps(touchingLeft, touchingRight),
        "touching rectangle edges must not become overlap near maximum int coordinates");

    ok &= Expect(!IsEditorToolAvailable(EditorTool::Select),
        "Select toolbar control must remain disabled until viewport selection exists");
    ok &= Expect(!IsEditorToolAvailable(EditorTool::Move),
        "Move toolbar control must remain disabled until transform editing exists");
    ok &= Expect(!IsEditorToolAvailable(EditorTool::Rotate),
        "Rotate toolbar control must remain disabled until transform editing exists");
    ok &= Expect(!IsEditorToolAvailable(EditorTool::Scale),
        "Scale toolbar control must remain disabled until transform editing exists");
    ok &= Expect(!IsEditorToolAvailable(EditorTool::Play),
        "Play toolbar control must remain disabled until play-in-editor exists");

    if (!ok) return EXIT_FAILURE;
    std::cout << "EditorLayoutTests: PASS\n";
    return EXIT_SUCCESS;
}
