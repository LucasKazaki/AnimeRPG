// Baseline JPEG (ITU T.81 sequential Huffman) decoder for imported textures.

#include "Engine/Assets/Gltf.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>

namespace Astral::Assets {

namespace {

constexpr int kZigZag[64] = {0, 1, 8, 16, 9, 2, 3, 10, 17, 24, 32, 25, 18, 11, 4, 5, 12, 19, 26, 33, 40, 48, 41, 34, 27,
    20, 13, 6, 7, 14, 21, 28, 35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51, 58, 59, 52, 45, 38, 31, 39,
    46, 53, 60, 61, 54, 47, 55, 62, 63};

struct Huffman {
    bool defined{};
    std::array<int, 17> minCode{}, maxCode{}, valuePointer{};
    std::array<std::uint8_t, 256> values{};

    bool Build(const std::uint8_t counts[16], const std::uint8_t* symbols, int total) {
        if (total > 256) return false;
        std::copy(symbols, symbols + total, values.begin());
        int code = 0, k = 0;
        for (int length = 1; length <= 16; ++length) {
            const int n = counts[length - 1];
            valuePointer[static_cast<std::size_t>(length)] = k;
            minCode[static_cast<std::size_t>(length)] = code;
            code += n;
            k += n;
            maxCode[static_cast<std::size_t>(length)] = n ? code - 1 : -1;
            if (code > (1 << length)) return false; // over-subscribed
            code <<= 1;
        }
        defined = true;
        return true;
    }
};

struct Component {
    int id{}, h{1}, v{1}, quant{};
    int dcTable{}, acTable{};
    int dcPredictor{};
    int planeWidth{}, planeHeight{};
    std::vector<std::uint8_t> plane;
};

class BitReader {
public:
    BitReader(const std::uint8_t* data, std::size_t size, std::size_t position) : data_(data), size_(size), pos_(position) {}
    int Bit() {
        if (count_ == 0) Fill();
        --count_;
        return static_cast<int>((buffer_ >> count_) & 1u);
    }
    int Bits(int n) {
        int value = 0;
        for (int i = 0; i < n; ++i) value = (value << 1) | Bit();
        return value;
    }
    void Align() { count_ = 0; }
    // Consumes an RSTn marker at the current position (after alignment).
    bool Restart() {
        Align();
        hitMarker_ = false;
        while (pos_ + 1 < size_ && data_[pos_] == 0xFF && data_[pos_ + 1] == 0xFF) ++pos_; // fill bytes
        if (pos_ + 1 < size_ && data_[pos_] == 0xFF && data_[pos_ + 1] >= 0xD0 && data_[pos_ + 1] <= 0xD7) {
            pos_ += 2;
            return true;
        }
        return false;
    }
    std::size_t Position() const { return pos_; }

private:
    void Fill() {
        std::uint8_t byte = 0;
        if (!hitMarker_ && pos_ < size_) {
            byte = data_[pos_];
            if (byte == 0xFF) {
                const std::uint8_t next = pos_ + 1 < size_ ? data_[pos_ + 1] : 0;
                if (next == 0x00) {
                    pos_ += 2;
                } else {
                    hitMarker_ = true; // a marker ends the entropy data; feed zeros
                    byte = 0;
                }
            } else {
                ++pos_;
            }
        }
        buffer_ = byte;
        count_ = 8;
    }
    const std::uint8_t* data_;
    std::size_t size_, pos_;
    std::uint32_t buffer_{};
    int count_{};
    bool hitMarker_{};
};

int DecodeSymbol(BitReader& bits, const Huffman& table) {
    int code = 0;
    for (int length = 1; length <= 16; ++length) {
        code = (code << 1) | bits.Bit();
        if (table.maxCode[static_cast<std::size_t>(length)] >= 0 && code <= table.maxCode[static_cast<std::size_t>(length)]) {
            const int index = table.valuePointer[static_cast<std::size_t>(length)] + code - table.minCode[static_cast<std::size_t>(length)];
            return index >= 0 && index < 256 ? table.values[static_cast<std::size_t>(index)] : -1;
        }
    }
    return -1;
}

int Extend(int value, int size) { return size == 0 ? 0 : (value < (1 << (size - 1)) ? value - (1 << size) + 1 : value); }

void InverseDct(const float in[64], std::uint8_t* out, int stride) {
    // Thread-safe one-time table (images decode concurrently on the job system).
    static const std::array<std::array<float, 8>, 8> cosines = [] {
        std::array<std::array<float, 8>, 8> table{};
        for (int x = 0; x < 8; ++x) {
            for (int u = 0; u < 8; ++u) {
                const float c = u == 0 ? 1.0f / std::sqrt(2.0f) : 1.0f;
                table[static_cast<std::size_t>(x)][static_cast<std::size_t>(u)]
                    = c * std::cos((2.0f * static_cast<float>(x) + 1.0f) * static_cast<float>(u) * 3.14159265358979f / 16.0f);
            }
        }
        return table;
    }();
    float temp[64];
    for (int y = 0; y < 8; ++y) { // rows: temp[y][x] = sum_u in[y][u] * c(x,u)
        for (int x = 0; x < 8; ++x) {
            float sum = 0.0f;
            for (int u = 0; u < 8; ++u) sum += in[y * 8 + u] * cosines[x][u];
            temp[y * 8 + x] = sum;
        }
    }
    for (int x = 0; x < 8; ++x) {
        for (int y = 0; y < 8; ++y) {
            float sum = 0.0f;
            for (int v = 0; v < 8; ++v) sum += temp[v * 8 + x] * cosines[y][v];
            const float value = sum * 0.25f + 128.0f;
            out[y * stride + x] = static_cast<std::uint8_t>(std::lround(std::min(255.0f, std::max(0.0f, value))));
        }
    }
}

} // namespace

bool DecodeJpeg(const std::uint8_t* data, std::size_t size, Graphics::ImageRgba8& out, std::string& error) {
    if (!data || size < 4 || data[0] != 0xFF || data[1] != 0xD8) {
        error = "not a JPEG";
        return false;
    }
    std::array<std::array<std::uint16_t, 64>, 4> quant{};
    std::array<bool, 4> quantDefined{};
    std::array<Huffman, 4> dcTables{}, acTables{};
    std::vector<Component> components;
    int width = 0, height = 0, restartInterval = 0;
    bool frameSeen = false;
    std::size_t pos = 2;
    auto segmentLength = [&](std::size_t at) -> std::size_t {
        return at + 1 < size ? (static_cast<std::size_t>(data[at]) << 8) | data[at + 1] : 0;
    };
    while (pos + 4 <= size) {
        if (data[pos] != 0xFF) {
            error = "expected a JPEG marker";
            return false;
        }
        const std::uint8_t marker = data[pos + 1];
        pos += 2;
        if (marker == 0xFF) {
            --pos; // fill byte
            continue;
        }
        if (marker == 0xD9) break; // EOI
        const std::size_t length = segmentLength(pos);
        if (length < 2 || pos + length > size) {
            error = "truncated JPEG segment";
            return false;
        }
        const std::uint8_t* segment = data + pos + 2;
        const std::size_t segmentSize = length - 2;
        switch (marker) {
        case 0xDB: { // DQT
            std::size_t i = 0;
            while (i < segmentSize) {
                const int precision = segment[i] >> 4, id = segment[i] & 15;
                ++i;
                const std::size_t bytes = precision ? 128u : 64u;
                if (id > 3 || i + bytes > segmentSize) {
                    error = "bad quantisation table";
                    return false;
                }
                for (int k = 0; k < 64; ++k) {
                    quant[static_cast<std::size_t>(id)][static_cast<std::size_t>(kZigZag[k])] = precision
                        ? static_cast<std::uint16_t>((segment[i + static_cast<std::size_t>(2 * k)] << 8) | segment[i + static_cast<std::size_t>(2 * k + 1)])
                        : segment[i + static_cast<std::size_t>(k)];
                }
                quantDefined[static_cast<std::size_t>(id)] = true;
                i += bytes;
            }
            break;
        }
        case 0xC4: { // DHT
            std::size_t i = 0;
            while (i + 17 <= segmentSize) {
                const int tableClass = segment[i] >> 4, id = segment[i] & 15;
                const std::uint8_t* counts = segment + i + 1;
                int total = 0;
                for (int k = 0; k < 16; ++k) total += counts[k];
                if (id > 3 || tableClass > 1 || i + 17 + static_cast<std::size_t>(total) > segmentSize) {
                    error = "bad Huffman table";
                    return false;
                }
                Huffman& table = tableClass == 0 ? dcTables[static_cast<std::size_t>(id)] : acTables[static_cast<std::size_t>(id)];
                if (!table.Build(counts, segment + i + 17, total)) {
                    error = "invalid Huffman code lengths";
                    return false;
                }
                i += 17 + static_cast<std::size_t>(total);
            }
            break;
        }
        case 0xC0:
        case 0xC1: { // SOF0/SOF1 baseline / extended sequential (8-bit)
            if (segmentSize < 6 || segment[0] != 8) {
                error = "only 8-bit baseline JPEG is supported";
                return false;
            }
            height = (segment[1] << 8) | segment[2];
            width = (segment[3] << 8) | segment[4];
            const int count = segment[5];
            if (width <= 0 || height <= 0 || width > 16384 || height > 16384 || (count != 1 && count != 3)
                || segmentSize < 6 + static_cast<std::size_t>(count) * 3) {
                error = "unsupported JPEG frame";
                return false;
            }
            components.resize(static_cast<std::size_t>(count));
            for (int c = 0; c < count; ++c) {
                Component& component = components[static_cast<std::size_t>(c)];
                component.id = segment[6 + c * 3];
                component.h = segment[7 + c * 3] >> 4;
                component.v = segment[7 + c * 3] & 15;
                component.quant = segment[8 + c * 3];
                if (component.h < 1 || component.h > 4 || component.v < 1 || component.v > 4 || component.quant > 3) {
                    error = "bad JPEG component";
                    return false;
                }
            }
            frameSeen = true;
            break;
        }
        case 0xC2:
        case 0xC3:
        case 0xC5:
        case 0xC6:
        case 0xC7:
        case 0xC9:
        case 0xCA:
        case 0xCB:
        case 0xCD:
        case 0xCE:
        case 0xCF:
            error = "progressive, lossless and arithmetic JPEG are not supported";
            return false;
        case 0xDD: // DRI
            if (segmentSize < 2) {
                error = "bad restart interval";
                return false;
            }
            restartInterval = (segment[0] << 8) | segment[1];
            break;
        case 0xDA: { // SOS: decode the (single, interleaved) scan
            if (!frameSeen) {
                error = "scan before frame";
                return false;
            }
            const int count = segmentSize > 0 ? segment[0] : 0;
            if (count != static_cast<int>(components.size()) || segmentSize < 1 + static_cast<std::size_t>(count) * 2 + 3) {
                error = "non-interleaved JPEG scans are not supported";
                return false;
            }
            for (int c = 0; c < count; ++c) {
                const int id = segment[1 + c * 2];
                const int tables = segment[2 + c * 2];
                auto it = std::find_if(components.begin(), components.end(), [id](const Component& k) { return k.id == id; });
                if (it == components.end()) {
                    error = "scan references an unknown component";
                    return false;
                }
                it->dcTable = tables >> 4;
                it->acTable = tables & 15;
                if (it->dcTable > 3 || it->acTable > 3 || !dcTables[static_cast<std::size_t>(it->dcTable)].defined
                    || !acTables[static_cast<std::size_t>(it->acTable)].defined || !quantDefined[static_cast<std::size_t>(it->quant)]) {
                    error = "scan uses an undefined table";
                    return false;
                }
            }
            int hMax = 1, vMax = 1;
            for (const Component& c : components) {
                hMax = std::max(hMax, c.h);
                vMax = std::max(vMax, c.v);
            }
            const int mcuWidth = 8 * hMax, mcuHeight = 8 * vMax;
            const int mcusX = (width + mcuWidth - 1) / mcuWidth, mcusY = (height + mcuHeight - 1) / mcuHeight;
            for (Component& c : components) {
                c.planeWidth = mcusX * c.h * 8;
                c.planeHeight = mcusY * c.v * 8;
                c.plane.assign(static_cast<std::size_t>(c.planeWidth) * static_cast<std::size_t>(c.planeHeight), 0);
                c.dcPredictor = 0;
            }
            BitReader bits(data, size, pos + length);
            int mcusUntilRestart = restartInterval;
            float block[64];
            for (int my = 0; my < mcusY; ++my) {
                for (int mx = 0; mx < mcusX; ++mx) {
                    if (restartInterval && mcusUntilRestart == 0) {
                        bits.Restart();
                        for (Component& c : components) c.dcPredictor = 0;
                        mcusUntilRestart = restartInterval;
                    }
                    for (Component& c : components) {
                        const auto& q = quant[static_cast<std::size_t>(c.quant)];
                        for (int by = 0; by < c.v; ++by) {
                            for (int bx = 0; bx < c.h; ++bx) {
                                std::fill(block, block + 64, 0.0f);
                                const int dcSize = DecodeSymbol(bits, dcTables[static_cast<std::size_t>(c.dcTable)]);
                                if (dcSize < 0 || dcSize > 11) {
                                    error = "corrupt JPEG DC coefficient";
                                    return false;
                                }
                                c.dcPredictor += Extend(bits.Bits(dcSize), dcSize);
                                block[0] = static_cast<float>(c.dcPredictor * q[0]);
                                for (int k = 1; k < 64;) {
                                    const int symbol = DecodeSymbol(bits, acTables[static_cast<std::size_t>(c.acTable)]);
                                    if (symbol < 0) {
                                        error = "corrupt JPEG AC coefficient";
                                        return false;
                                    }
                                    const int run = symbol >> 4, acSize = symbol & 15;
                                    if (acSize == 0) {
                                        if (run != 15) break; // end of block
                                        k += 16;
                                        continue;
                                    }
                                    k += run;
                                    if (k > 63) {
                                        error = "JPEG coefficient index overflow";
                                        return false;
                                    }
                                    const int zz = kZigZag[k];
                                    block[zz] = static_cast<float>(Extend(bits.Bits(acSize), acSize) * q[static_cast<std::size_t>(zz)]);
                                    ++k;
                                }
                                const int x0 = (mx * c.h + bx) * 8, y0 = (my * c.v + by) * 8;
                                InverseDct(block, &c.plane[static_cast<std::size_t>(y0) * static_cast<std::size_t>(c.planeWidth) + static_cast<std::size_t>(x0)], c.planeWidth);
                            }
                        }
                    }
                    if (restartInterval) --mcusUntilRestart;
                }
            }
            // Colour conversion with nearest-neighbour chroma upsampling.
            out.Resize(width, height);
            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                    auto sample = [&](const Component& c) {
                        const int sx = x * c.h / hMax, sy = y * c.v / vMax;
                        return static_cast<float>(c.plane[static_cast<std::size_t>(sy) * static_cast<std::size_t>(c.planeWidth) + static_cast<std::size_t>(sx)]);
                    };
                    std::uint8_t r, g, b;
                    if (components.size() == 1) {
                        r = g = b = static_cast<std::uint8_t>(sample(components[0]));
                    } else {
                        const float Y = sample(components[0]), cb = sample(components[1]) - 128.0f, cr = sample(components[2]) - 128.0f;
                        auto clamp8 = [](float v) { return static_cast<std::uint8_t>(std::lround(std::min(255.0f, std::max(0.0f, v)))); };
                        r = clamp8(Y + 1.402f * cr);
                        g = clamp8(Y - 0.344136f * cb - 0.714136f * cr);
                        b = clamp8(Y + 1.772f * cb);
                    }
                    out.Set(x, y, {r, g, b, 255});
                }
            }
            return true;
        }
        default: break; // APPn, COM and others are skipped
        }
        pos += length;
    }
    error = frameSeen ? "JPEG has no scan" : "JPEG has no baseline frame";
    return false;
}

} // namespace Astral::Assets
