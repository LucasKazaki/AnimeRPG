# Astral / AnimeRPG Art Bible v0.1

**Status:** proposed art direction, not Lucas-approved production lock.  
**Loop:** `astral-art-hourly-20260922`  
**Date:** 2026-09-22

## Visual thesis

Astral should read as **recognizable contemporary Washington, DC interrupted by an impossible supernatural energy system**. The ordinary world stays materially grounded: pale civic stone, dark asphalt, restrained metals, glass, mature greenery, warm indoor light and believable public-space wear. Mana is the exception. It supplies the strongest chroma, edge light, motion and shape distortion, so supernatural events remain readable without turning every surface into glowing fantasy decoration.

Reference games are quality references only. Current official PlayStation material describes Genshin Impact around large readable environments, real-time rendering, changing light/weather and finely tuned character animation, while Zenless Zone Zero emphasizes lively urban neighborhoods and a distinct audiovisual identity. Astral adapts those goals as environmental legibility, character readability, neighborhood identity and polished motion, not copied palettes, costumes, UI, maps or effects.

## Proposed palette hierarchy

This palette is a look-development constraint, not a permanent shader constant.

| Role | Proposed color | Use |
|---|---|---|
| Civic limestone | `#C8BEA1` | Monument stone, quiet architecture reference |
| Graphite | `#20242A` | UI darks, equipment neutrals, shadow mass |
| Asphalt | `#34373A` | Roads, service spaces, grounded contrast |
| Vegetation | `#52694E` | Natural midtone, never neon by default |
| Ion teal | `#42D9C8` | Primary mana/readability accent |
| Ultraviolet | `#8068F2` | Secondary supernatural state, depth/class variation |
| Vermilion | `#F15B52` | Danger, hostile telegraph, damage-critical state |
| Rare-state gold | `#E6C878` | Mastery, rare reward, high-value UI cue |

Rules:

1. Ordinary environment materials stay mostly low to medium saturation.
2. Mana accents occupy less screen area than neutral world color outside major abilities/portals.
3. Danger is communicated by shape, timing and motion as well as vermilion, never color alone.
4. Reward gold is reserved for permanent progression and exceptional states.

## Character shape language

The game has one persistent customizable protagonist. Invest in a flexible base body, face, hair, equipment and class silhouette system instead of collectible-character churn.

- Proposed adult protagonist proportion: about 7.5 to 8 heads tall until gameplay-camera and rig tests prove it.
- Preserve readable head, hands and weapon silhouette at gameplay distance; large shape/value groups outrank tiny costume detail.
- Male and female options should share one gameplay skeleton wherever deformation quality permits.
- Customization may change identity, but each class keeps recognizable large silhouette anchors.

| Class | Shape rule | Motion/effect rule |
|---|---|---|
| Shadowblade | Tapered, forward-leaning, asymmetric accents, narrow weapon shapes | Short directional streaks, compressed anticipation, sharp release, low linger |
| Arc Mage | Vertical/open silhouette, layered panels, clear casting-hand space | Expanding geometric fields, controlled arcs, readable centers/boundaries |
| Aegis | Broad rectangular torso/guard shapes, lower visual mass | Thick defensive volumes, slower expansion, obvious protected-zone edges |

Color alone must never be the only class identifier.

## Material language

Surface variation exists at three scales: silhouette/form, mid-frequency wear/joints and restrained microdetail. Do not use dense procedural noise to disguise weak modeling.

- Base color contains material color variation but no directional light or painted ambient shadow.
- Pure nonmetals use metallic 0. Pure exposed metal uses metallic 1. Intermediate values require a real mixed surface.
- Roughness carries most small reflection variation and should not become random static.
- Tangent-space normals use the positive-Y convention defined by the starter-material contract and proposed glTF path.
- Repeated architecture needs seamless tiling, scale metadata and future macro variation.
- Common construction should favor reusable material families and trim/tiling systems once supported.

## Toon and lighting direction

The target is stylized real-time rendering, not flat unlit anime and not photorealism with anime heads added afterward.

- Character skin/cloth favor controlled light bands and clean value separation, starting from physically coherent material inputs.
- Outlines are optional and must be stable; the character still has to read without them.
- Baseline art review uses neutral daylight with fixed exposure plus an overcast check.
- Rim light is a readability accent, not a permanent white halo.
- Preserve readable dark values and avoid crushed-black costumes.

## Environment composition

The National Mall must remain recognizable through proportion, axes, spacing and landmark hierarchy before detail density. DC's civic order is the visual baseline; supernatural intrusions deliberately break it.

- Use long sightlines and strong landmark silhouettes for orientation.
- Keep traversal and encounter spaces readable from gameplay height, not just cinematic cameras.
- Reuse paving, curbs, stairs, rails, benches, barriers, lights, trees, stone families and service elements.
- Separate geographic accuracy from artistic simplification in the asset ledger.
- Shadow Crypt contrasts through compressed space, repeated structural rhythm and directional mana intrusion.
- Mana Reactor contrasts through engineered modules, visible mechanical function, conduits and controlled high-energy zones rather than random sci-fi panel noise.

## VFX readability

Every combat effect communicates a gameplay fact before spectacle.

1. Anticipation shows where the action begins.
2. Active shape shows the damaging/protective volume.
3. Dissipation is shorter and lower contrast unless the lingering volume remains mechanically relevant.
4. Friendly, hostile and neutral effects differ in shape/motion as well as color.
5. Persistent opaque effects should not hide the protagonist, enemy wind-up or camera center in normal combat.
6. Test effects in grayscale and reduced saturation to catch color-only signaling.

## UI and typography direction

Use restrained institutional clarity with supernatural accents. The base layer should feel like a precise modern system, not fantasy parchment.

- Use a legible sans-serif family with verified redistribution rights before shipping.
- Reserve display type for headings/supernatural states, not body text.
- Critical combat UI needs shape/icon redundancy for color-blind readability.
- Motion reinforces hierarchy/confirmation; avoid constant idle movement in core HUD elements.

## Review views required

A production asset is not art-approved from one flattering render. Capture the relevant set: neutral orthographic/3-4 views, gameplay-camera silhouette, daylight and overcast material views, topology/UV views, deformation poses, 3x3 tiled material view plus grazing light, and runtime captures labeled with engine revision.

## Reference evidence used for v0.1

Accessed 2026-09-22:

- PlayStation, Genshin Impact: https://www.playstation.com/en-us/games/genshin-impact/
- PlayStation Store, Genshin Impact: https://store.playstation.com/en-au/concept/10000896
- PlayStation, Zenless Zone Zero: https://www.playstation.com/en-us/games/zenless-zone-zero/
- Epic UE 5.8 PBR: https://dev.epicgames.com/documentation/unreal-engine/physically-based-materials-in-unreal-engine
- Unity 6 URP Lit: https://docs.unity3d.com/6000.0/Manual/urp/prebuilt-shader-graphs-urp-lit.html
- Khronos glTF 2.0.1: https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html

These references support quality targets and material-channel conventions. They do not grant rights to copy game or engine assets.
