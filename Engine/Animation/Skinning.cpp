#include "Engine/Animation/Skinning.h"

#include <cstddef>

namespace Astral::Animation {

using namespace Math;

void ComputeSkinMatrices(const Skeleton& skeleton, const Pose& pose, std::vector<Mat4>& skin) {
    std::vector<Mat4> model;
    pose.ToModel(skeleton, model);
    const std::vector<Mat4>& inverseBind = skeleton.InverseBindMatrices();
    skin.resize(model.size());
    for (std::size_t i = 0; i < model.size(); ++i) {
        skin[i] = i < inverseBind.size() ? model[i] * inverseBind[i] : model[i];
    }
}

void SkinMesh(const Graphics::MeshData& bind, const std::vector<Mat4>& skin, Graphics::MeshData& out) {
    if (out.vertices.size() != bind.vertices.size() || out.indices.size() != bind.indices.size()) {
        out = bind;
    }
    const bool skinned = bind.skin.size() == bind.vertices.size() && !skin.empty();
    out.bounds = AABB{};
    for (std::size_t v = 0; v < bind.vertices.size(); ++v) {
        const Graphics::Vertex& source = bind.vertices[v];
        Graphics::Vertex& target = out.vertices[v];
        target.uv = source.uv;
        target.color = source.color;
        if (!skinned) {
            target.position = source.position;
            target.normal = source.normal;
        } else {
            Vec3 position{}, normal{};
            const Graphics::SkinInfluence& influence = bind.skin[v];
            for (int i = 0; i < 4; ++i) {
                const float w = influence.weights[static_cast<std::size_t>(i)];
                if (w <= 0.0f) continue;
                const std::size_t joint = influence.joints[static_cast<std::size_t>(i)];
                const Mat4& m = joint < skin.size() ? skin[joint] : skin[0];
                position += TransformPoint(m, source.position) * w;
                normal += TransformVector(m, source.normal) * w;
            }
            target.position = position;
            target.normal = Normalize(normal, source.normal);
        }
        out.bounds.Expand(target.position);
    }
}

} // namespace Astral::Animation
