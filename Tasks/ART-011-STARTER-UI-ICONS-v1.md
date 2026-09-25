# ART-011: Starter UI Icons v1

**Loop:** `astral-art-hourly-20260922`  
**Owner:** art worker  
**Status:** proposed source fixture, not runtime acceptance

## Goal

Add a small original, generic 2D icon source pack for menus, HUD prototypes, tutorials, and editor-facing starter content. The pack is source art only. It does not claim Astral SVG import, UI rendering, atlasing, rasterization, or editor registration.

## Reference rationale

- W3C SVG 2 defines a scalable XML vector-graphics source format suitable for deterministic, inspectable 2D source art.
- Unreal Engine 5.8 UMG exposes reusable image widgets and Slate-brush based UI presentation, reinforcing the need for reusable icon assets without requiring Astral to copy Unreal's implementation.
- Unity 6 documentation recommends raster textures/sprites for UI Toolkit production use while treating SVG/vector support as a separate import path. Therefore SVG is used here only as editable source, with raster/runtime derivatives deferred until Astral's actual UI texture contract is known.

No Epic, Unity, or third-party art is copied.

## Source contract

- one original SVG contact-sheet/source library
- 12 unique monochrome functional icons
- 24 x 24 unit authoring grid per icon
- 2 unit primary stroke, round caps and joins
- `currentColor` or `none` only, so source art has no baked theme color
- no scripts, external references, fonts, embedded raster images, animation, or executable content
- minimum 2 unit optical safe margin for core marks where practical
- source -> validated -> imported -> runtime_verified -> art_approved remain separate states

## Verification stop condition

This bounded pass is source-validated when:

1. the SVG parses as XML,
2. exactly 12 expected symbol IDs exist,
3. each symbol uses `viewBox="0 0 24 24"`,
4. every contact-sheet instance references a local expected symbol exactly once,
5. no prohibited SVG elements or external hrefs exist,
6. no hard-coded colors beyond `currentColor` and `none` exist,
7. the expected manifest pins exact byte/hash/count metadata,
8. source QA records those checks.

## Not claimed

No Astral SVG support, texture import, sprite atlas, DPI policy, UI renderer integration, runtime capture, platform accessibility certification, UE5/Unity parity, or independent visual-art approval.

## Allowed paths

This bounded ART-011 packet owns only:

- `Content/Starter/UIIconsV1/**`
- `Docs/QA/ART-011-*`
- `Docs/Agents/art-hourly/BACKLOG.json`
- `Docs/Agents/art-hourly/runs/*art011*`
- `Tasks/ART-011-STARTER-UI-ICONS-v1.md`

Renderer, editor, gameplay, build/CI, native runtime, dependency, and other workers' paths are out of scope.

## Current pass evidence

Prepared original source asset: `starter-ui-icons.svg` with 12 functional symbols (play, pause, stop, check, close, warning, info, settings, eye, lock, search, menu).

- prepared SVG SHA-256: `6532d497f710c2818e1cadbc4d1683c676c6001fca73f17254a68bec41dc5fbf`
- prepared SVG bytes: `4029`
- XML/source-structure checks: passed
- unique expected symbols: `12/12`
- local contact-sheet references: `12/12`
- 24 x 24 symbol viewBoxes: `12/12`
- prohibited active/external elements: none observed
- 1:1 offline raster sanity: all 12 cells visible; zero ink pixels touched any cell's outermost 1-pixel perimeter
- independent visual self-check corrected the initial settings mark from a sun-like radial symbol to a recognizable gear silhouette before pinning the hash
- Astral import/runtime/art approval: not run and not claimed

Publication of the source contract and SVG under `Content/Starter/UIIconsV1/**` was attempted after this task packet was published, but the connected GitHub write path rejected the content write. No alternate or low-level bypass was used. The source package remains preserved as a handoff pending a permitted repository write.

