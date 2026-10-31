#pragma once

#include <cstdint>
#include <string_view>
#include <span>

namespace ExtenDeck::Utilities::FNV1a_64 {
    constexpr uint64_t prime64 = 1099511628211ull;
    constexpr uint64_t offset64 = 14695981039346656037ull;

    constexpr inline uint64_t hash64(std::string_view str) noexcept {
        uint64_t hash = offset64;
        for (unsigned char c : str) {
            hash ^= static_cast<uint64_t>(c);
            hash *= prime64;
        }
        return hash;
        
    }
}
