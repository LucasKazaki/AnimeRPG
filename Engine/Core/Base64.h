#pragma once

// RFC 4648 base64 (standard alphabet, '=' padding), used for data URIs in
// glTF and JSON-embedded binary payloads.

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace Astral::Core {

inline std::string Base64Encode(const std::uint8_t* data, std::size_t size) {
    static const char* kAlphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    out.reserve((size + 2) / 3 * 4);
    for (std::size_t i = 0; i < size; i += 3) {
        const std::uint32_t a = data[i];
        const std::uint32_t b = i + 1 < size ? data[i + 1] : 0u;
        const std::uint32_t c = i + 2 < size ? data[i + 2] : 0u;
        const std::uint32_t triple = (a << 16) | (b << 8) | c;
        out.push_back(kAlphabet[(triple >> 18) & 63u]);
        out.push_back(kAlphabet[(triple >> 12) & 63u]);
        out.push_back(i + 1 < size ? kAlphabet[(triple >> 6) & 63u] : '=');
        out.push_back(i + 2 < size ? kAlphabet[triple & 63u] : '=');
    }
    return out;
}

// Ignores ASCII whitespace; rejects other characters and bad padding.
inline bool Base64Decode(std::string_view text, std::vector<std::uint8_t>& out) {
    out.clear();
    out.reserve(text.size() / 4 * 3);
    std::uint32_t buffer = 0;
    int bits = 0;
    std::size_t padding = 0;
    for (const char c : text) {
        if (c == ' ' || c == '\n' || c == '\r' || c == '\t') continue;
        int value;
        if (c >= 'A' && c <= 'Z') value = c - 'A';
        else if (c >= 'a' && c <= 'z') value = c - 'a' + 26;
        else if (c >= '0' && c <= '9') value = c - '0' + 52;
        else if (c == '+' || c == '-') value = 62; // also accept the URL-safe alphabet
        else if (c == '/' || c == '_') value = 63;
        else if (c == '=') {
            ++padding;
            continue;
        } else {
            return false;
        }
        if (padding > 0) return false; // data after padding
        buffer = (buffer << 6) | static_cast<std::uint32_t>(value);
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            out.push_back(static_cast<std::uint8_t>((buffer >> bits) & 0xFFu));
        }
    }
    return padding <= 2 && bits < 6;
}

} // namespace Astral::Core
