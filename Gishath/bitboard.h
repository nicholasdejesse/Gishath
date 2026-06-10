#pragma once
#include "types.h"

constexpr Bitboard shift_BB(Bitboard b, Direction d) {
    switch (d) {
        case north: return b << 8;
        case south: return b >> 8;
        case east: return (b << 1) & ~file_a_bb;
        case west: return (b >> 1) & ~file_h_bb;
        case north_east: return (b << 9) & ~file_a_bb;
        case north_west: return (b << 7) & ~file_h_bb;
        case south_east: return (b >> 7) & ~file_a_bb;
        case south_west: return (b >> 9) & ~file_h_bb;
    }
    return 0;
}

constexpr int popcount(Bitboard b) {
    int count = 0;
    while (b) {
        b &= b - 1; // Clear the least significant bit set
        ++count;
    }
    return count;
}