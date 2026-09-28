#pragma once

// Linear blend skinning on the CPU (up to four influences per vertex).

#include "Engine/Animation/Skeleton.h"
#include "Engine/Graphics/Mesh.h"

#include <vector>

namespace Astral::Animation {

// skin[j] = model[j] * inverseBind[j]
void ComputeSkinMatrices(const Skeleton& skeleton, const Pose& pose, std::vector<Mat4>& skin);

// Deforms `bind` into `out` (topology is copied on first use / size change).
// Vertices without skin data are passed through unchanged.
void SkinMesh(const Graphics::MeshData& bind, const std::vector<Mat4>& skin, Graphics::MeshData& out);

} // namespace Astral::Animation
