#include "Engine/Graphics/Image.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <utility>

namespace Astral::Graphics {

// ------------------------------------------------------------------ ImageRgba8

void ImageRgba8::Resize(int newWidth, int newHeight, Rgba8 fill) {
    width = std::max(0, newWidth);
    height = std::max(0, newHeight);
    pixels.assign(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4u, 0);
    for (std::size_t i = 0; i < pixels.size(); i += 4) {
        pixels[i] = fill.r;
        pixels[i + 1] = fill.g;
        pixels[i + 2] = fill.b;
        pixels[i + 3] = fill.a;
    }
}

Rgba8 ImageRgba8::Get(int x, int y) const {
    if (!Contains(x, y)) return {};
    const std::size_t i = (static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x)) * 4u;
    return {pixels[i], pixels[i + 1], pixels[i + 2], pixels[i + 3]};
}

void ImageRgba8::Set(int x, int y, Rgba8 value) {
    if (!Contains(x, y)) return;
    const std::size_t i = (static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x)) * 4u;
    pixels[i] = value.r;
    pixels[i + 1] = value.g;
    pixels[i + 2] = value.b;
    pixels[i + 3] = value.a;
}

void ImageRgba8::Blend(int x, int y, Rgba8 value, float alpha) {
    if (!Contains(x, y)) return;
    alpha = Math::Saturate(alpha * (value.a / 255.0f));
    const std::size_t i = (static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x)) * 4u;
    if (pixels[i + 3] == 255) {
        // Opaque destination (every render target): plain lerp, alpha stays 255.
        auto mix = [alpha](std::uint8_t dst, std::uint8_t src) {
            return static_cast<std::uint8_t>(std::lround(dst + (src - dst) * alpha));
        };
        pixels[i] = mix(pixels[i], value.r);
        pixels[i + 1] = mix(pixels[i + 1], value.g);
        pixels[i + 2] = mix(pixels[i + 2], value.b);
        return;
    }
    // Source-over with straight (non-premultiplied) alpha.
    const float dstAlpha = pixels[i + 3] / 255.0f;
    const float outAlpha = alpha + dstAlpha * (1.0f - alpha);
    if (outAlpha <= 0.0f) return;
    auto compose = [&](std::uint8_t dst, std::uint8_t src) {
        const float c = (static_cast<float>(src) * alpha + static_cast<float>(dst) * dstAlpha * (1.0f - alpha)) / outAlpha;
        return static_cast<std::uint8_t>(std::lround(Math::Clamp(c, 0.0f, 255.0f)));
    };
    pixels[i] = compose(pixels[i], value.r);
    pixels[i + 1] = compose(pixels[i + 1], value.g);
    pixels[i + 2] = compose(pixels[i + 2], value.b);
    pixels[i + 3] = static_cast<std::uint8_t>(std::lround(outAlpha * 255.0f));
}

std::uint64_t HashImage(const ImageRgba8& image) {
    std::uint64_t hash = 1469598103934665603ull;
    auto mix = [&hash](std::uint8_t byte) {
        hash ^= byte;
        hash *= 1099511628211ull;
    };
    for (int shift = 0; shift < 32; shift += 8) {
        mix(static_cast<std::uint8_t>(static_cast<std::uint32_t>(image.width) >> shift));
        mix(static_cast<std::uint8_t>(static_cast<std::uint32_t>(image.height) >> shift));
    }
    for (std::uint8_t byte : image.pixels) mix(byte);
    return hash;
}

// ------------------------------------------------------------------ checksums

std::uint32_t Crc32(const std::uint8_t* data, std::size_t size, std::uint32_t crc) {
    static const std::array<std::uint32_t, 256> table = [] {
        std::array<std::uint32_t, 256> values{};
        for (std::uint32_t n = 0; n < 256; ++n) {
            std::uint32_t c = n;
            for (int k = 0; k < 8; ++k) c = (c & 1u) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
            values[n] = c;
        }
        return values;
    }();
    crc = ~crc;
    for (std::size_t i = 0; i < size; ++i) crc = table[(crc ^ data[i]) & 0xFFu] ^ (crc >> 8);
    return ~crc;
}

namespace {

std::uint32_t Adler32(const std::uint8_t* data, std::size_t size) {
    std::uint32_t a = 1, b = 0;
    while (size > 0) {
        const std::size_t chunk = std::min<std::size_t>(size, 5552);
        for (std::size_t i = 0; i < chunk; ++i) {
            a += data[i];
            b += a;
        }
        a %= 65521u;
        b %= 65521u;
        data += chunk;
        size -= chunk;
    }
    return (b << 16) | a;
}

void PutBigEndian32(std::vector<std::uint8_t>& out, std::uint32_t value) {
    out.push_back(static_cast<std::uint8_t>(value >> 24));
    out.push_back(static_cast<std::uint8_t>(value >> 16));
    out.push_back(static_cast<std::uint8_t>(value >> 8));
    out.push_back(static_cast<std::uint8_t>(value));
}

std::uint32_t GetBigEndian32(const std::uint8_t* p) {
    return (static_cast<std::uint32_t>(p[0]) << 24) | (static_cast<std::uint32_t>(p[1]) << 16)
        | (static_cast<std::uint32_t>(p[2]) << 8) | static_cast<std::uint32_t>(p[3]);
}

constexpr std::array<std::uint16_t, 29> kLengthBase{{3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27,
    31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258}};
constexpr std::array<std::uint8_t, 29> kLengthExtra{{0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3,
    3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0}};
constexpr std::array<std::uint16_t, 30> kDistanceBase{{1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97,
    129, 193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577}};
constexpr std::array<std::uint8_t, 30> kDistanceExtra{{0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7,
    7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13}};

// ------------------------------------------------------------------ deflate encoder

class BitWriter {
public:
    explicit BitWriter(std::vector<std::uint8_t>& out) : out_(out) {}
    void Write(std::uint32_t bits, int count) {
        buffer_ |= bits << count_;
        count_ += count;
        while (count_ >= 8) {
            out_.push_back(static_cast<std::uint8_t>(buffer_ & 0xFFu));
            buffer_ >>= 8;
            count_ -= 8;
        }
    }
    void WriteCode(std::uint32_t code, int length) {
        std::uint32_t reversed = 0;
        for (int i = 0; i < length; ++i) reversed |= ((code >> i) & 1u) << (length - 1 - i);
        Write(reversed, length);
    }
    void Flush() {
        if (count_ > 0) out_.push_back(static_cast<std::uint8_t>(buffer_ & 0xFFu));
        buffer_ = 0;
        count_ = 0;
    }

private:
    std::vector<std::uint8_t>& out_;
    std::uint32_t buffer_{};
    int count_{};
};

void WriteLiteralLength(BitWriter& writer, int symbol) {
    if (symbol <= 143) writer.WriteCode(0x30u + static_cast<std::uint32_t>(symbol), 8);
    else if (symbol <= 255) writer.WriteCode(0x190u + static_cast<std::uint32_t>(symbol - 144), 9);
    else if (symbol <= 279) writer.WriteCode(static_cast<std::uint32_t>(symbol - 256), 7);
    else writer.WriteCode(0xC0u + static_cast<std::uint32_t>(symbol - 280), 8);
}

void WriteMatch(BitWriter& writer, int length, int distance) {
    int lengthIndex = 28;
    while (kLengthBase[static_cast<std::size_t>(lengthIndex)] > length) --lengthIndex;
    WriteLiteralLength(writer, 257 + lengthIndex);
    writer.Write(static_cast<std::uint32_t>(length - kLengthBase[static_cast<std::size_t>(lengthIndex)]),
        kLengthExtra[static_cast<std::size_t>(lengthIndex)]);
    int distanceIndex = 29;
    while (kDistanceBase[static_cast<std::size_t>(distanceIndex)] > distance) --distanceIndex;
    writer.WriteCode(static_cast<std::uint32_t>(distanceIndex), 5);
    writer.Write(static_cast<std::uint32_t>(distance - kDistanceBase[static_cast<std::size_t>(distanceIndex)]),
        kDistanceExtra[static_cast<std::size_t>(distanceIndex)]);
}

// ------------------------------------------------------------------ inflate decoder

class BitReader {
public:
    BitReader(const std::uint8_t* data, std::size_t size) : data_(data), size_(size) {}
    bool Bits(int count, std::uint32_t& value) {
        while (count_ < count) {
            if (position_ >= size_) return false;
            buffer_ |= static_cast<std::uint32_t>(data_[position_++]) << count_;
            count_ += 8;
        }
        value = buffer_ & ((count == 32) ? 0xFFFFFFFFu : ((1u << count) - 1u));
        buffer_ = count == 32 ? 0 : buffer_ >> count;
        count_ -= count;
        return true;
    }
    void AlignToByte() {
        buffer_ = 0;
        count_ = 0;
    }
    std::size_t Position() const { return position_; }
    const std::uint8_t* Data() const { return data_; }
    std::size_t Size() const { return size_; }
    void Skip(std::size_t bytes) { position_ += bytes; }

private:
    const std::uint8_t* data_;
    std::size_t size_;
    std::size_t position_{};
    std::uint32_t buffer_{};
    int count_{};
};

struct Huffman {
    std::array<std::uint16_t, 16> counts{};
    std::vector<std::uint16_t> symbols;
};

// Returns false for an over-subscribed code (incomplete codes are allowed and
// fail only if an unused code is actually read).
bool BuildHuffman(Huffman& h, const std::uint8_t* lengths, int symbolCount) {
    h.counts.fill(0);
    for (int s = 0; s < symbolCount; ++s) ++h.counts[lengths[s]];
    int left = 1;
    for (int length = 1; length <= 15; ++length) {
        left <<= 1;
        left -= h.counts[static_cast<std::size_t>(length)];
        if (left < 0) return false;
    }
    std::array<std::uint16_t, 16> offsets{};
    for (int length = 1; length < 15; ++length)
        offsets[static_cast<std::size_t>(length + 1)] = static_cast<std::uint16_t>(
            offsets[static_cast<std::size_t>(length)] + h.counts[static_cast<std::size_t>(length)]);
    h.symbols.assign(static_cast<std::size_t>(symbolCount), 0);
    for (int s = 0; s < symbolCount; ++s) {
        if (lengths[s] != 0) h.symbols[offsets[lengths[s]]++] = static_cast<std::uint16_t>(s);
    }
    return true;
}

bool Decode(BitReader& reader, const Huffman& h, int& symbol) {
    int code = 0, first = 0, index = 0;
    for (int length = 1; length <= 15; ++length) {
        std::uint32_t bit = 0;
        if (!reader.Bits(1, bit)) return false;
        code |= static_cast<int>(bit);
        const int count = h.counts[static_cast<std::size_t>(length)];
        if (code - count < first) {
            const std::size_t at = static_cast<std::size_t>(index + (code - first));
            if (at >= h.symbols.size()) return false;
            symbol = h.symbols[at];
            return true;
        }
        index += count;
        first += count;
        first <<= 1;
        code <<= 1;
    }
    return false;
}

bool InflateCodes(BitReader& reader, const Huffman& lengthCode, const Huffman& distanceCode,
    std::vector<std::uint8_t>& out, std::size_t maxOutput, std::string& error) {
    for (;;) {
        int symbol = 0;
        if (!Decode(reader, lengthCode, symbol)) {
            error = "deflate: invalid literal/length code";
            return false;
        }
        if (symbol < 256) {
            if (out.size() >= maxOutput) {
                error = "deflate: output exceeds limit";
                return false;
            }
            out.push_back(static_cast<std::uint8_t>(symbol));
            continue;
        }
        if (symbol == 256) return true;
        symbol -= 257;
        if (symbol >= 29) {
            error = "deflate: invalid length symbol";
            return false;
        }
        std::uint32_t extra = 0;
        if (!reader.Bits(kLengthExtra[static_cast<std::size_t>(symbol)], extra)) {
            error = "deflate: truncated length";
            return false;
        }
        const std::size_t length = kLengthBase[static_cast<std::size_t>(symbol)] + extra;
        int distanceSymbol = 0;
        if (!Decode(reader, distanceCode, distanceSymbol) || distanceSymbol >= 30) {
            error = "deflate: invalid distance code";
            return false;
        }
        if (!reader.Bits(kDistanceExtra[static_cast<std::size_t>(distanceSymbol)], extra)) {
            error = "deflate: truncated distance";
            return false;
        }
        const std::size_t distance = kDistanceBase[static_cast<std::size_t>(distanceSymbol)] + extra;
        if (distance > out.size()) {
            error = "deflate: distance before start of output";
            return false;
        }
        if (out.size() + length > maxOutput) {
            error = "deflate: output exceeds limit";
            return false;
        }
        const std::size_t from = out.size() - distance;
        for (std::size_t i = 0; i < length; ++i) out.push_back(out[from + i]);
    }
}

bool Inflate(BitReader& reader, std::vector<std::uint8_t>& out, std::size_t maxOutput, std::string& error) {
    static const std::pair<Huffman, Huffman> fixed = [] {
        std::array<std::uint8_t, 288> lengths{};
        for (int i = 0; i < 144; ++i) lengths[static_cast<std::size_t>(i)] = 8;
        for (int i = 144; i < 256; ++i) lengths[static_cast<std::size_t>(i)] = 9;
        for (int i = 256; i < 280; ++i) lengths[static_cast<std::size_t>(i)] = 7;
        for (int i = 280; i < 288; ++i) lengths[static_cast<std::size_t>(i)] = 8;
        Huffman lengthCode, distanceCode;
        BuildHuffman(lengthCode, lengths.data(), 288);
        std::array<std::uint8_t, 30> distances{};
        distances.fill(5);
        BuildHuffman(distanceCode, distances.data(), 30);
        return std::make_pair(lengthCode, distanceCode);
    }();
    std::uint32_t last = 0;
    do {
        std::uint32_t type = 0;
        if (!reader.Bits(1, last) || !reader.Bits(2, type)) {
            error = "deflate: truncated block header";
            return false;
        }
        if (type == 0) {
            reader.AlignToByte();
            const std::size_t at = reader.Position();
            if (at + 4 > reader.Size()) {
                error = "deflate: truncated stored header";
                return false;
            }
            const std::uint8_t* p = reader.Data() + at;
            const unsigned length = p[0] | (p[1] << 8);
            const unsigned inverse = p[2] | (p[3] << 8);
            if ((length ^ 0xFFFFu) != inverse) {
                error = "deflate: stored length mismatch";
                return false;
            }
            if (at + 4 + length > reader.Size()) {
                error = "deflate: truncated stored data";
                return false;
            }
            if (out.size() + length > maxOutput) {
                error = "deflate: output exceeds limit";
                return false;
            }
            out.insert(out.end(), p + 4, p + 4 + length);
            reader.Skip(4 + length);
        } else if (type == 1) {
            if (!InflateCodes(reader, fixed.first, fixed.second, out, maxOutput, error)) return false;
        } else if (type == 2) {
            std::uint32_t hlit = 0, hdist = 0, hclen = 0;
            if (!reader.Bits(5, hlit) || !reader.Bits(5, hdist) || !reader.Bits(4, hclen)) {
                error = "deflate: truncated dynamic header";
                return false;
            }
            hlit += 257;
            hdist += 1;
            hclen += 4;
            if (hlit > 286 || hdist > 30) {
                error = "deflate: bad dynamic code counts";
                return false;
            }
            static const std::array<std::uint8_t, 19> order{{16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15}};
            std::array<std::uint8_t, 19> codeLengths{};
            for (std::uint32_t i = 0; i < hclen; ++i) {
                std::uint32_t value = 0;
                if (!reader.Bits(3, value)) {
                    error = "deflate: truncated code lengths";
                    return false;
                }
                codeLengths[order[i]] = static_cast<std::uint8_t>(value);
            }
            Huffman codeLengthCode;
            if (!BuildHuffman(codeLengthCode, codeLengths.data(), 19)) {
                error = "deflate: bad code-length code";
                return false;
            }
            std::array<std::uint8_t, 320> lengths{};
            std::uint32_t index = 0;
            while (index < hlit + hdist) {
                int symbol = 0;
                if (!Decode(reader, codeLengthCode, symbol)) {
                    error = "deflate: bad code length symbol";
                    return false;
                }
                if (symbol < 16) {
                    lengths[index++] = static_cast<std::uint8_t>(symbol);
                    continue;
                }
                std::uint8_t repeated = 0;
                std::uint32_t repeat = 0;
                if (symbol == 16) {
                    if (index == 0) {
                        error = "deflate: repeat with no previous length";
                        return false;
                    }
                    repeated = lengths[index - 1];
                    if (!reader.Bits(2, repeat)) return error = "deflate: truncated repeat", false;
                    repeat += 3;
                } else if (symbol == 17) {
                    if (!reader.Bits(3, repeat)) return error = "deflate: truncated repeat", false;
                    repeat += 3;
                } else {
                    if (!reader.Bits(7, repeat)) return error = "deflate: truncated repeat", false;
                    repeat += 11;
                }
                if (index + repeat > hlit + hdist) {
                    error = "deflate: code lengths overflow";
                    return false;
                }
                while (repeat-- > 0) lengths[index++] = repeated;
            }
            if (lengths[256] == 0) {
                error = "deflate: missing end-of-block code";
                return false;
            }
            Huffman lengthCode, distanceCode;
            if (!BuildHuffman(lengthCode, lengths.data(), static_cast<int>(hlit))
                || !BuildHuffman(distanceCode, lengths.data() + hlit, static_cast<int>(hdist))) {
                error = "deflate: over-subscribed dynamic code";
                return false;
            }
            if (!InflateCodes(reader, lengthCode, distanceCode, out, maxOutput, error)) return false;
        } else {
            error = "deflate: reserved block type";
            return false;
        }
    } while (!last);
    return true;
}

int PaethPredictor(int a, int b, int c) {
    const int p = a + b - c;
    const int pa = std::abs(p - a), pb = std::abs(p - b), pc = std::abs(p - c);
    if (pa <= pb && pa <= pc) return a;
    return pb <= pc ? b : c;
}

void AppendChunk(std::vector<std::uint8_t>& png, const char type[4], const std::vector<std::uint8_t>& data) {
    PutBigEndian32(png, static_cast<std::uint32_t>(data.size()));
    const std::size_t typeStart = png.size();
    png.insert(png.end(), type, type + 4);
    png.insert(png.end(), data.begin(), data.end());
    PutBigEndian32(png, Crc32(png.data() + typeStart, data.size() + 4));
}

} // namespace

std::vector<std::uint8_t> ZlibCompress(const std::uint8_t* data, std::size_t size) {
    std::vector<std::uint8_t> out{0x78, 0x9C};
    BitWriter writer(out);
    writer.Write(1, 1); // final block
    writer.Write(1, 2); // fixed Huffman
    // static: lambdas below use these without capturing them (MSVC rejects
    // implicit use of a non-static local constexpr in a capture-list lambda).
    static constexpr int kWindow = 32768;
    static constexpr int kHashSize = 1 << 15;
    static constexpr int kMaxChain = 48;
    std::vector<int> head(kHashSize, -1);
    std::vector<int> previous(kWindow, -1);
    auto hashAt = [data](std::size_t i) {
        return static_cast<int>(((data[i] << 10) ^ (data[i + 1] << 5) ^ data[i + 2]) & (kHashSize - 1));
    };
    auto insert = [&](std::size_t i) {
        if (i + 2 >= size) return;
        const int h = hashAt(i);
        previous[i & (kWindow - 1)] = head[static_cast<std::size_t>(h)];
        head[static_cast<std::size_t>(h)] = static_cast<int>(i);
    };
    std::size_t position = 0;
    while (position < size) {
        int bestLength = 0;
        int bestDistance = 0;
        if (position + 2 < size) {
            int candidate = head[static_cast<std::size_t>(hashAt(position))];
            const int maxLength = static_cast<int>(std::min<std::size_t>(258, size - position));
            for (int chain = 0; candidate >= 0 && chain < kMaxChain; ++chain) {
                const int distance = static_cast<int>(position) - candidate;
                if (distance <= 0 || distance > kWindow) break;
                int length = 0;
                while (length < maxLength && data[static_cast<std::size_t>(candidate + length)] == data[position + static_cast<std::size_t>(length)])
                    ++length;
                if (length > bestLength) {
                    bestLength = length;
                    bestDistance = distance;
                    if (length == maxLength) break;
                }
                const int next = previous[static_cast<std::size_t>(candidate) & (kWindow - 1)];
                if (next >= candidate) break;
                candidate = next;
            }
        }
        if (bestLength >= 3) {
            WriteMatch(writer, bestLength, bestDistance);
            for (int i = 0; i < bestLength; ++i) insert(position + static_cast<std::size_t>(i));
            position += static_cast<std::size_t>(bestLength);
        } else {
            WriteLiteralLength(writer, data[position]);
            insert(position);
            ++position;
        }
    }
    WriteLiteralLength(writer, 256);
    writer.Flush();
    PutBigEndian32(out, Adler32(data, size));
    return out;
}

bool ZlibDecompress(const std::uint8_t* data, std::size_t size, std::vector<std::uint8_t>& out,
    std::size_t maxOutputBytes, std::string& error) {
    out.clear();
    if (size < 6) {
        error = "zlib: stream too short";
        return false;
    }
    const unsigned cmf = data[0], flg = data[1];
    if ((cmf & 0x0Fu) != 8 || (cmf >> 4) > 7 || ((cmf << 8) | flg) % 31u != 0 || (flg & 0x20u)) {
        error = "zlib: unsupported or corrupt header";
        return false;
    }
    BitReader reader(data + 2, size - 2);
    if (!Inflate(reader, out, maxOutputBytes, error)) return false;
    const std::size_t trailer = 2 + reader.Position();
    if (trailer + 4 > size) {
        error = "zlib: missing checksum";
        return false;
    }
    if (GetBigEndian32(data + trailer) != Adler32(out.data(), out.size())) {
        error = "zlib: checksum mismatch";
        return false;
    }
    return true;
}

std::vector<std::uint8_t> EncodePng(const ImageRgba8& image, bool includeAlpha) {
    const int channels = includeAlpha ? 4 : 3;
    const std::size_t stride = static_cast<std::size_t>(image.width) * static_cast<std::size_t>(channels);
    std::vector<std::uint8_t> raw(static_cast<std::size_t>(image.height) * stride);
    for (int y = 0; y < image.height; ++y) {
        for (int x = 0; x < image.width; ++x) {
            const Rgba8 p = image.Get(x, y);
            std::uint8_t* dst = &raw[static_cast<std::size_t>(y) * stride + static_cast<std::size_t>(x * channels)];
            dst[0] = p.r;
            dst[1] = p.g;
            dst[2] = p.b;
            if (includeAlpha) dst[3] = p.a;
        }
    }
    // Per-row adaptive filtering (minimum sum of absolute signed residuals).
    std::vector<std::uint8_t> filtered;
    filtered.reserve(static_cast<std::size_t>(image.height) * (stride + 1));
    std::vector<std::uint8_t> candidate(stride);
    std::vector<std::uint8_t> best(stride);
    for (int y = 0; y < image.height; ++y) {
        const std::uint8_t* row = &raw[static_cast<std::size_t>(y) * stride];
        const std::uint8_t* up = y > 0 ? row - stride : nullptr;
        long bestScore = -1;
        std::uint8_t bestType = 0;
        for (std::uint8_t type = 0; type <= 4; ++type) {
            long score = 0;
            for (std::size_t i = 0; i < stride; ++i) {
                const int a = i >= static_cast<std::size_t>(channels) ? row[i - static_cast<std::size_t>(channels)] : 0;
                const int b = up ? up[i] : 0;
                const int c = (up && i >= static_cast<std::size_t>(channels)) ? up[i - static_cast<std::size_t>(channels)] : 0;
                int predicted = 0;
                switch (type) {
                case 1: predicted = a; break;
                case 2: predicted = b; break;
                case 3: predicted = (a + b) / 2; break;
                case 4: predicted = PaethPredictor(a, b, c); break;
                default: break;
                }
                const std::uint8_t value = static_cast<std::uint8_t>(row[i] - predicted);
                candidate[i] = value;
                score += value < 128 ? value : 256 - value;
            }
            if (bestScore < 0 || score < bestScore) {
                bestScore = score;
                bestType = type;
                best.swap(candidate);
            }
        }
        filtered.push_back(bestType);
        filtered.insert(filtered.end(), best.begin(), best.end());
    }
    std::vector<std::uint8_t> png{0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
    std::vector<std::uint8_t> header;
    PutBigEndian32(header, static_cast<std::uint32_t>(image.width));
    PutBigEndian32(header, static_cast<std::uint32_t>(image.height));
    header.push_back(8);
    header.push_back(includeAlpha ? 6 : 2);
    header.push_back(0);
    header.push_back(0);
    header.push_back(0);
    AppendChunk(png, "IHDR", header);
    AppendChunk(png, "IDAT", ZlibCompress(filtered.data(), filtered.size()));
    AppendChunk(png, "IEND", {});
    return png;
}

bool WritePng(const std::string& path, const ImageRgba8& image, std::string& error, bool includeAlpha) {
    if (image.width <= 0 || image.height <= 0) {
        error = "cannot write an empty image";
        return false;
    }
    const std::vector<std::uint8_t> png = EncodePng(image, includeAlpha);
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) {
        error = "cannot open " + path;
        return false;
    }
    file.write(reinterpret_cast<const char*>(png.data()), static_cast<std::streamsize>(png.size()));
    if (!file) {
        error = "failed writing " + path;
        return false;
    }
    return true;
}

bool DecodePng(const std::uint8_t* data, std::size_t size, ImageRgba8& out, std::string& error,
    const PngLimits& limits) {
    out = ImageRgba8{};
    static const std::uint8_t kSignature[8] = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
    if (size > limits.maxFileBytes) return error = "png: file exceeds limit", false;
    if (size < 8 || !std::equal(kSignature, kSignature + 8, data)) return error = "png: bad signature", false;
    std::size_t position = 8;
    std::uint32_t width = 0, height = 0;
    int colorType = -1;
    bool sawHeader = false, sawEnd = false;
    std::vector<std::uint8_t> idat;
    std::vector<Rgba8> palette;
    while (position + 12 <= size) {
        const std::uint32_t length = GetBigEndian32(data + position);
        if (length > size - position - 12) return error = "png: truncated chunk", false;
        const std::uint8_t* type = data + position + 4;
        const std::uint8_t* body = data + position + 8;
        if (GetBigEndian32(body + length) != Crc32(type, length + 4u)) return error = "png: chunk CRC mismatch", false;
        const std::string name(reinterpret_cast<const char*>(type), 4);
        if (!sawHeader && name != "IHDR") return error = "png: IHDR must come first", false;
        if (name == "IHDR") {
            if (sawHeader || length != 13) return error = "png: bad IHDR", false;
            sawHeader = true;
            width = GetBigEndian32(body);
            height = GetBigEndian32(body + 4);
            const int bitDepth = body[8];
            colorType = body[9];
            if (width == 0 || height == 0 || width > static_cast<std::uint32_t>(limits.maxDimension)
                || height > static_cast<std::uint32_t>(limits.maxDimension)
                || static_cast<std::size_t>(width) * height > limits.maxPixels)
                return error = "png: dimensions out of range", false;
            if (bitDepth != 8) return error = "png: only 8-bit channels are supported", false;
            if (colorType != 0 && colorType != 2 && colorType != 3 && colorType != 4 && colorType != 6)
                return error = "png: unsupported colour type", false;
            if (body[10] != 0 || body[11] != 0) return error = "png: unsupported compression/filter method", false;
            if (body[12] != 0) return error = "png: interlaced images are not supported", false;
        } else if (name == "PLTE") {
            if (length % 3 != 0 || length / 3 > 256 || length == 0) return error = "png: bad palette", false;
            palette.clear();
            for (std::uint32_t i = 0; i < length; i += 3) palette.push_back({body[i], body[i + 1], body[i + 2], 255});
        } else if (name == "tRNS") {
            if (colorType == 3) {
                for (std::uint32_t i = 0; i < length && i < palette.size(); ++i) palette[i].a = body[i];
            }
        } else if (name == "IDAT") {
            idat.insert(idat.end(), body, body + length);
        } else if (name == "IEND") {
            sawEnd = true;
            break;
        } else if (!(type[0] & 0x20)) {
            return error = "png: unknown critical chunk " + name, false;
        }
        position += 12u + length;
    }
    if (!sawHeader || !sawEnd) return error = "png: missing IHDR or IEND", false;
    if (colorType == 3 && palette.empty()) return error = "png: palette image without PLTE", false;
    const int channels = colorType == 0 ? 1 : colorType == 2 ? 3 : colorType == 3 ? 1 : colorType == 4 ? 2 : 4;
    const std::size_t stride = static_cast<std::size_t>(width) * static_cast<std::size_t>(channels);
    const std::size_t expected = static_cast<std::size_t>(height) * (stride + 1);
    std::vector<std::uint8_t> raw;
    if (!ZlibDecompress(idat.data(), idat.size(), raw, expected, error)) return false;
    if (raw.size() != expected) return error = "png: decompressed size mismatch", false;
    std::vector<std::uint8_t> previous(stride, 0), current(stride, 0);
    out.Resize(static_cast<int>(width), static_cast<int>(height));
    for (std::uint32_t y = 0; y < height; ++y) {
        const std::uint8_t* row = &raw[y * (stride + 1)];
        const std::uint8_t filter = row[0];
        if (filter > 4) return error = "png: bad filter type", false;
        for (std::size_t i = 0; i < stride; ++i) {
            const int a = i >= static_cast<std::size_t>(channels) ? current[i - static_cast<std::size_t>(channels)] : 0;
            const int b = previous[i];
            const int c = i >= static_cast<std::size_t>(channels) ? previous[i - static_cast<std::size_t>(channels)] : 0;
            int predicted = 0;
            switch (filter) {
            case 1: predicted = a; break;
            case 2: predicted = b; break;
            case 3: predicted = (a + b) / 2; break;
            case 4: predicted = PaethPredictor(a, b, c); break;
            default: break;
            }
            current[i] = static_cast<std::uint8_t>(row[1 + i] + predicted);
        }
        for (std::uint32_t x = 0; x < width; ++x) {
            const std::uint8_t* p = &current[static_cast<std::size_t>(x) * static_cast<std::size_t>(channels)];
            Rgba8 pixel;
            switch (colorType) {
            case 0: pixel = {p[0], p[0], p[0], 255}; break;
            case 2: pixel = {p[0], p[1], p[2], 255}; break;
            case 3:
                if (p[0] >= palette.size()) return error = "png: palette index out of range", false;
                pixel = palette[p[0]];
                break;
            case 4: pixel = {p[0], p[0], p[0], p[1]}; break;
            default: pixel = {p[0], p[1], p[2], p[3]}; break;
            }
            out.Set(static_cast<int>(x), static_cast<int>(y), pixel);
        }
        previous.swap(current);
    }
    return true;
}

bool ReadPng(const std::string& path, ImageRgba8& out, std::string& error, const PngLimits& limits) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        error = "cannot open " + path;
        return false;
    }
    file.seekg(0, std::ios::end);
    const std::streamoff length = file.tellg();
    if (length < 0 || static_cast<std::size_t>(length) > limits.maxFileBytes) {
        error = "png: file exceeds limit";
        return false;
    }
    file.seekg(0, std::ios::beg);
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(length));
    if (length > 0) file.read(reinterpret_cast<char*>(bytes.data()), length);
    if (!file) {
        error = "failed reading " + path;
        return false;
    }
    return DecodePng(bytes.data(), bytes.size(), out, error, limits);
}

} // namespace Astral::Graphics
