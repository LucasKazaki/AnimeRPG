# Native GPU fixture

NativeGpu preserves the original Astral neutral fixture authored October 18,
2026 for native GPU consumer acceptance. Geometry is the original FacetedMarker;
its two materials use KHR_materials_unlit. The authored scene submits four draws,
76 vertices, 22 triangles and one checker texture. This is a procedural engine
verification fixture, not production game art.

The five files were recovered unchanged from the original local fixture and
verified against its fixture-authoring receipt. Geometry SHA-256:
7b522670ae131cc50db2978f6604468066d3c3c660caddb84f4685a2493e2e24.

EngineD3D11Tests copies these inputs to a unique temporary directory before
mutating them. Its capture evidence goes to a separate unique build directory.
Checking this fixture in removes the dependency on the original author's local
evidence folder. Hardware correctness, native presentation, resize/reload,
live-object lifetime and performance assertions are unchanged.
