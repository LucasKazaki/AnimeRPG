# Astral color and exposure reference v0.1

**Status:** proposed art-direction calibration, not a renderer implementation or final color-grade lock.  
**Loop:** `astral-art-hourly-20260922`  
**Date:** 2026-09-23

## Decision

Separate **asset color**, **scene lighting/exposure**, and **creative color grading**. Artists should not bake a dramatic scene grade or directional lighting into base-color textures to compensate for an unfinished renderer. The proposed palette remains an sRGB communication/reference layer. Runtime materials, lighting, exposure and tonemapping must be evaluated in the engine's documented working space once that path exists.

Epic's Unreal Engine 5.8 documentation treats film/tonemapper settings as project-wide look controls and recommends doing artistic correction in scene-referred linear space rather than depending on an LDR LUT. Unity 6 URP likewise exposes Neutral and ACES tonemapping, and its HDR guidance calls out display paper-white/brightness calibration. Astral should therefore make the neutral review state reproducible before any stylized grade is accepted.

## Review states

Every material, character and environment look-development review should eventually capture the same asset in at least these states:

1. **Neutral daylight:** fixed white balance, fixed exposure, no creative LUT, neutral background. Used for material/color acceptance.
2. **Overcast:** lower directional contrast, still fixed exposure. Used to catch shape/detail that only works under strong key light.
3. **Mana-event:** controlled supernatural emissive/effect accents. Used to test whether gameplay silhouettes survive spectacle.

Until Astral exposes these controls, all three states are requirements, not runtime evidence.

## Reference pack

`Content/Calibration/ColorValue/` defines eight proposed semantic colors already present in the art bible. The generator emits:

- `palette_card.png`: exact top-half semantic swatches, relative-luminance preview blocks, and a 16-step code-value ladder;
- `neutral_lut_16.png`: a deterministic neutral 16x16x16 cube unwrapped to 256x16 with **Astral's own documented layout** `x = R + 16*B`, `y = G`;
- `value_ramp_16.png`: 16 exact sRGB grayscale patches.

The grayscale preview is computed by decoding each sRGB role color to linear light, calculating relative luminance, and re-encoding that luminance as neutral sRGB gray. It is intended to reveal broad value separation. It is not a perceptual-lightness model and should not be used as a substitute for grayscale playtesting.

The LUT is a diagnostic source reference only. It is **not** claimed compatible with Unreal, Unity, Astral, OCIO, ACES, or any particular shader until an importer/render test says so.

## Palette rules

The eight proposed roles remain civic limestone, graphite, asphalt, vegetation, ion teal, ultraviolet, vermilion and rare-state gold. Their hex values are display/reference sRGB values, not scene-linear shader constants.

Critical UI text-like pairings on graphite are screened at 4.5:1 using the WCAG relative-luminance formula. This is a useful accessibility screen, not a claim that the game UI conforms to WCAG by color contrast alone. Game HUD states still need icon/shape redundancy, scaling, motion/readability tests and actual implementation review.

Mana, hostile danger and rewards must still be identifiable when hue information is weak. The relative-luminance preview intentionally shows that some different semantic accents can approach one another in value. Shape, timing, iconography and motion remain mandatory secondary channels.

## Capture metadata required later

A runtime art-review capture should record engine commit, scene ID, camera transform/FOV, resolution, display mode (SDR/HDR), working/output color space, tonemapper name/version, exposure/white balance, post-process overrides, light types/intensities, material IDs, and whether the image is native Astral output or an offline DCC/reference render.

Do not use an unlabeled offline render as runtime acceptance.

## Primary references checked 2026-09-23

- Epic, Unreal Engine 5.8, Color Grading and the Filmic Tonemapper: https://dev.epicgames.com/documentation/unreal-engine/color-grading-and-the-filmic-tonemapper-in-unreal-engine
- Unity 6 URP, Tonemapping volume override and HDR output guidance: https://docs.unity3d.com/6000.0/Manual/urp/post-processing-tonemapping.html
- Unity 6 URP, Color Adjustments: https://docs.unity3d.com/6000.0/Manual/urp/Post-Processing-Color-Adjustments.html
- W3C WCAG 2.2 relative-luminance definition and contrast guidance: https://www.w3.org/WAI/WCAG21/Understanding/relative-luminance.html
