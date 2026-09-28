#pragma once

// PCG32 (O'Neill 2014): small, fast, statistically strong and fully
// deterministic across compilers, unlike std::uniform_*_distribution.

#include <cstdint>

namespace Astral::Core {

class Random {
public:
    explicit Random(std::uint64_t seed = 0x853c49e6748fea9bull, std::uint64_t stream = 0xda3e39cb94b95bdbull) {
        Seed(seed, stream);
    }
    void Seed(std::uint64_t seed, std::uint64_t stream = 0xda3e39cb94b95bdbull) {
        state_ = 0u;
        increment_ = (stream << 1u) | 1u;
        NextU32();
        state_ += seed;
        NextU32();
    }
    std::uint32_t NextU32() {
        const std::uint64_t old = state_;
        state_ = old * 6364136223846793005ull + increment_;
        const std::uint32_t xorShifted = static_cast<std::uint32_t>(((old >> 18u) ^ old) >> 27u);
        const std::uint32_t rotation = static_cast<std::uint32_t>(old >> 59u);
        return (xorShifted >> rotation) | (xorShifted << ((32u - rotation) & 31u));
    }
    // Uniform in [0, 1).
    float NextFloat() { return static_cast<float>(NextU32() >> 8) * (1.0f / 16777216.0f); }
    float Range(float low, float high) { return low + (high - low) * NextFloat(); }
    // Uniform integer in [0, bound) without modulo bias.
    std::uint32_t NextBounded(std::uint32_t bound) {
        if (bound == 0) return 0;
        const std::uint32_t threshold = (0u - bound) % bound;
        for (;;) {
            const std::uint32_t value = NextU32();
            if (value >= threshold) return value % bound;
        }
    }

private:
    std::uint64_t state_{};
    std::uint64_t increment_{};
};

// Stateless integer hash for deterministic per-element noise (e.g. VFX seeds).
inline std::uint32_t Hash32(std::uint32_t x) {
    x ^= x >> 16; x *= 0x7feb352du;
    x ^= x >> 15; x *= 0x846ca68bu;
    x ^= x >> 16;
    return x;
}

} // namespace Astral::Core
