#include "ShortcutView/SHA256.hpp"

#include <cstring>
#include <sstream>
#include <iomanip>

// thanks chatgpt
void SHA256::update(const uint8_t* data, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        m_data[m_datalen++] = data[i];
        if (m_datalen == 64) {
            transform();
            m_bitlen += 512;
            m_datalen = 0;
        }
    }
}

void SHA256::update(const std::string& s) {
    update(reinterpret_cast<const uint8_t*>(s.c_str()), s.size());
}

std::array<uint8_t, 32> SHA256::digest() {
    std::array<uint8_t, 32> hash{};

    size_t i = m_datalen;

    // Pad whatever data is left in the buffer.
    if (m_datalen < 56) {
        m_data[i++] = 0x80;
        while (i < 56) m_data[i++] = 0x00;
    } else {
        m_data[i++] = 0x80;
        while (i < 64) m_data[i++] = 0x00;
        transform();
        memset(m_data, 0, 56);
    }

    m_bitlen += m_datalen * 8;
    m_data[63] = m_bitlen;
    m_data[62] = m_bitlen >> 8;
    m_data[61] = m_bitlen >> 16;
    m_data[60] = m_bitlen >> 24;
    m_data[59] = m_bitlen >> 32;
    m_data[58] = m_bitlen >> 40;
    m_data[57] = m_bitlen >> 48;
    m_data[56] = m_bitlen >> 56;
    transform();

    for (i = 0; i < 4; ++i) {
        for (int j = 0; j < 8; ++j) {
            hash[i + (j * 4)] = (m_state[j] >> (24 - i * 8)) & 0x000000ff;
        }
    }

    return hash;
}

std::string SHA256::toString(const std::array<uint8_t, 32>& digest) {
    std::stringstream ss;
    for (uint8_t b : digest) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)b;
    }
    return ss.str();
}

uint32_t SHA256::rotr(uint32_t x, uint32_t n) {
    return (x >> n) | (x << (32 - n));
}

void SHA256::transform() {
    uint32_t a, b, c, d, e, f, g, h, t1, t2;
    uint32_t m[64];

    for (uint32_t i = 0, j = 0; i < 16; ++i, j += 4)
        m[i] = (m_data[j] << 24) | (m_data[j + 1] << 16) | (m_data[j + 2] << 8) | (m_data[j + 3]);
    for (uint32_t i = 16; i < 64; ++i) {
        uint32_t s0 = rotr(m[i - 15], 7) ^ rotr(m[i - 15], 18) ^ (m[i - 15] >> 3);
        uint32_t s1 = rotr(m[i - 2], 17) ^ rotr(m[i - 2], 19) ^ (m[i - 2] >> 10);
        m[i] = m[i - 16] + s0 + m[i - 7] + s1;
    }

    a = m_state[0]; b = m_state[1]; c = m_state[2]; d = m_state[3];
    e = m_state[4]; f = m_state[5]; g = m_state[6]; h = m_state[7];

    for (uint32_t i = 0; i < 64; ++i) {
        uint32_t S1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
        uint32_t ch = (e & f) ^ (~e & g);
        t1 = h + S1 + ch + k[i] + m[i];
        uint32_t S0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
        uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
        t2 = S0 + maj;

        h = g; g = f; f = e;
        e = d + t1;
        d = c; c = b; b = a;
        a = t1 + t2;
    }

    m_state[0] += a;
    m_state[1] += b;
    m_state[2] += c;
    m_state[3] += d;
    m_state[4] += e;
    m_state[5] += f;
    m_state[6] += g;
    m_state[7] += h;
}
