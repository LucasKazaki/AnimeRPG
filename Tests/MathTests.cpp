#include "Engine/Math/Math.h"

#include <cassert>
#include <cmath>
#include <limits>

int main() {
    const Astral::Math::Vec3 vector{3.0f, 4.0f, 0.0f};
    assert(std::fabs(vector.Length() - 5.0f) < 0.001f);

    const Astral::Math::Vec3 huge{1e20f, 1e20f, 0.0f};
    assert(std::isfinite(huge.Length()));
    assert(std::fabs(huge.Length() / 1e20f - std::sqrt(2.0f)) < 1e-6f);
    const Astral::Math::Vec3 tiny{1e-30f, 0.0f, 0.0f};
    assert(tiny.Length() == 1e-30f);
    assert(Astral::Math::Vec3{}.Length() == 0.0f);
    const float limit = std::numeric_limits<float>::max();
    assert(std::isinf((Astral::Math::Vec3{limit, limit, limit}.Length())));
    assert(Astral::Math::IsFinite(huge));
    assert(Astral::Math::IsFinite(tiny));
    assert(!Astral::Math::IsFinite({std::numeric_limits<float>::quiet_NaN(), 0.0f, 0.0f}));
    assert(!Astral::Math::IsFinite({0.0f, std::numeric_limits<float>::infinity(), 0.0f}));

    const Astral::Math::Mat4 identity = Astral::Math::Mat4::Identity();
    for (int index = 0; index < 4; ++index) {
        assert(identity.m[index][index] == 1.0f);
    }
    return 0;
}
