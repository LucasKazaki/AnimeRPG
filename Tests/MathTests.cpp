#include "Engine/Math/Math.h"

#include <cassert>
#include <cmath>

int main() {
    const Astral::Math::Vec3 vector{3.0f, 4.0f, 0.0f};
    assert(std::fabs(vector.Length() - 5.0f) < 0.001f);

    const Astral::Math::Mat4 identity = Astral::Math::Mat4::Identity();
    for (int index = 0; index < 4; ++index) {
        assert(identity.m[index][index] == 1.0f);
    }
    return 0;
}
