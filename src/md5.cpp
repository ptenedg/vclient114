#include "md5.h"
#include <cmath>
#include <cstdint>
#include <vector>

namespace {
inline uint32_t rotl(uint32_t x, int c) { return (x << c) | (x >> (32 - c)); }
const int S[64] = {7,12,17,22,7,12,17,22,7,12,17,22,7,12,17,22,
                   5,9,14,20,5,9,14,20,5,9,14,20,5,9,14,20,
                   4,11,16,23,4,11,16,23,4,11,16,23,4,11,16,23,
                   6,10,15,21,6,10,15,21,6,10,15,21,6,10,15,21};
}

std::string md5Hex(const std::string& data) {
    uint32_t K[64];
    for (int i = 0; i < 64; ++i)
        K[i] = static_cast<uint32_t>(std::floor(std::fabs(std::sin(i + 1.0)) * 4294967296.0));

    uint32_t a0 = 0x67452301, b0 = 0xefcdab89, c0 = 0x98badcfe, d0 = 0x10325476;

    std::vector<uint8_t> msg(data.begin(), data.end());
    uint64_t bitLen = static_cast<uint64_t>(data.size()) * 8;
    msg.push_back(0x80);
    while (msg.size() % 64 != 56) msg.push_back(0);
    for (int i = 0; i < 8; ++i) msg.push_back(static_cast<uint8_t>(bitLen >> (8 * i)));

    for (size_t off = 0; off < msg.size(); off += 64) {
        uint32_t M[16];
        for (int i = 0; i < 16; ++i)
            M[i] = msg[off + 4*i] | (msg[off + 4*i + 1] << 8) |
                   (msg[off + 4*i + 2] << 16) | (static_cast<uint32_t>(msg[off + 4*i + 3]) << 24);
        uint32_t A = a0, B = b0, C = c0, D = d0;
        for (int i = 0; i < 64; ++i) {
            uint32_t F; int g;
            if (i < 16)      { F = (B & C) | (~B & D); g = i; }
            else if (i < 32) { F = (D & B) | (~D & C); g = (5*i + 1) % 16; }
            else if (i < 48) { F = B ^ C ^ D;          g = (3*i + 5) % 16; }
            else             { F = C ^ (B | ~D);       g = (7*i) % 16; }
            F = F + A + K[i] + M[g];
            A = D; D = C; C = B;
            B = B + rotl(F, S[i]);
        }
        a0 += A; b0 += B; c0 += C; d0 += D;
    }

    static const char* hex = "0123456789ABCDEF";
    std::string out;
    for (uint32_t v : {a0, b0, c0, d0})
        for (int i = 0; i < 4; ++i) {
            uint8_t byte = static_cast<uint8_t>(v >> (8 * i));
            out += hex[byte >> 4];
            out += hex[byte & 0xF];
        }
    return out;
}
