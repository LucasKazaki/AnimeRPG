# National Mall reference ledger

Status: **source/reference only**. These files do not contain game-ready meshes, textures, map tiles, scans, measured drawings, or runtime evidence.

`reference-ledger.json` is the environment-art source ledger for the first National Mall blockout/material pass. It records only authoritative URLs, compact modeling facts, source measurements, art-use intent, and simplification constraints. The current policy is deliberately conservative: page media is not copied into the repository, and a source URL does not grant redistribution rights for photographs, drawings, maps, or downloadable assets on that page.

The ledger supports the proposed Astral art bible by separating five reference classes:

- `axis_scale`: landmark alignment and major spacing that should survive blockout simplification;
- `materials`: source-backed stone/metal distinctions that should not be flattened into one generic civic material;
- `landscape`: grade, water and composition rules for spaces such as Constitution Gardens;
- `vegetation`: canopy/allee rhythm and species-family context before high-detail foliage;
- `context_boundary`: adjacent east/museum edges that establish orientation but still need per-building source packets before detailed modeling.

Source measurements remain in the units used by the authoritative page. Convert them to metres at the authoring/import boundary and keep the original value in provenance. Any gameplay-driven compression or rerouting should be documented as an intentional simplification rather than silently presented as survey accuracy.

## Authoritative source set, retrieved 2026-09-23

- National Park Service, The Mall: https://www.nps.gov/places/000/national-mall.htm
- National Park Service, Lincoln Memorial Event Operations Guide, reflecting-pool dimensions and paths: https://www.nps.gov/nama/planyourvisit/upload/20241129_LINC-Event-Operations-Guidelines-FINAL-2.pdf
- National Park Service, Lincoln Memorial Building Statistics: https://www.nps.gov/linc/learn/historyculture/lincoln-memorial-building-statistics.htm
- National Park Service, Washington Monument construction reference: https://www.nps.gov/articles/000/build-your-own-washington-monument.htm
- National Park Service, Constitution Gardens Cultural Landscape: https://www.nps.gov/articles/600012.htm
- National Park Service, Vietnam Veterans Memorial overview: https://www.nps.gov/vive/learn/historyculture/vvmoverview.htm
- National Park Service, World War II Memorial FAQ: https://www.nps.gov/wwii/faqs.htm
- National Park Service, World War II Memorial place page: https://www.nps.gov/places/national-world-war-ii-memorial.htm
- National Park Service, The Mall Cultural Landscape: https://www.nps.gov/articles/600213.htm
- Architect of the Capitol, U.S. Capitol Grounds: https://www.aoc.gov/explore-capitol-campus/buildings-grounds/capitol-grounds
- Smithsonian Institution, Museum Maps: https://www.si.edu/visit/maps

No page image or downloadable source asset was adopted in this pass. Item-level rights review remains mandatory before any external media enters the repository.
