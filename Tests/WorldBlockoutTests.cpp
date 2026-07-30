#include "Engine/Scene/WorldBlockout.h"

#include <cmath>
#include <cstdlib>

namespace {
void Check(bool condition) {
    if (!condition) std::abort();
}

bool Equal(float left, float right) {
    return std::fabs(left - right) < 0.001f;
}
}

int main() {
    using namespace Astral::Scene;
    const WorldBlockout first;
    const WorldBlockout second;
    Check(first.Landmarks().size() == 3);
    for (std::size_t index = 0; index < first.Landmarks().size(); ++index) {
        const LandmarkProxy& left = first.Landmarks()[index];
        const LandmarkProxy& right = second.Landmarks()[index];
        Check(left.kind == right.kind);
        Check(Equal(left.position.x, right.position.x));
        Check(Equal(left.position.y, right.position.y));
        Check(Equal(left.position.z, right.position.z));
        Check(Equal(left.dimensions.x, right.dimensions.x));
        Check(Equal(left.dimensions.y, right.dimensions.y));
        Check(Equal(left.dimensions.z, right.dimensions.z));
        Check(left.dimensions.x > 0.0f && left.dimensions.y > 0.0f
            && left.dimensions.z > 0.0f);
        Check(left.position.z >= first.Grid().minimumZ);
        Check(left.position.z <= first.Grid().maximumZ);
        if (index > 0) Check(left.position.z != first.Landmarks()[index - 1].position.z);
    }

    Check(first.Landmarks()[0].kind == LandmarkKind::LincolnMemorial);
    Check(first.Landmarks()[1].kind == LandmarkKind::ReflectingPool);
    Check(first.Landmarks()[2].kind == LandmarkKind::WashingtonMonument);
    Check(first.Grid().spacing > 0.0f);
    Check(first.Grid().minimumX < first.Grid().maximumX);
    Check(first.Grid().minimumZ < first.Grid().maximumZ);

    const Astral::Math::Vec3 mapped = first.GroundPosition({3.0f, 9.0f, 27.0f});
    Check(Equal(mapped.x, 3.0f));
    Check(Equal(mapped.y, 0.0f));
    Check(Equal(mapped.z, 9.0f));
    return 0;
}
