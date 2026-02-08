#pragma once

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using s8 = int8_t;
using s16 = int16_t;
using s32 = int32_t;
using s64 = int64_t;

//using sprite_tile = std::array<std::array<u8, 8>, 8>;
using sprite_tile = std::vector<std::vector<u8>>;
using pattern_table = std::vector<std::vector<u8>>;
using nametable = std::array<u8, 1024>; // takes up 1kB
using display = std::array<std::array<u8, 256>, 240>; // takes up 1kB

using palette = std::array<u8, 32>;