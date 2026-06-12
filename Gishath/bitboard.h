#pragma once
#include <bit>
#include "types.h"

constexpr Bitboard shift_BB(Bitboard bb, Direction d) {
    switch (d) {
        case north: return bb << 8;
        case south: return bb >> 8;
        case east: return (bb << 1) & ~file_bb_of(file_a);
        case west: return (bb >> 1) & ~file_bb_of(file_h);
        case north_east: return (bb << 9) & ~file_bb_of(file_a);
        case north_west: return (bb << 7) & ~file_bb_of(file_h);
        case south_east: return (bb >> 7) & ~file_bb_of(file_a);
        case south_west: return (bb >> 9) & ~file_bb_of(file_h);
    }
    return 0;
}

constexpr Bitboard pawn_push_BB(Bitboard bb, Color us) {
    return shift_BB(bb, relative_direction(us, north));
}

Square pop_lsb(Bitboard& bb);
Square get_lsb(Bitboard bb);

constexpr int popcount(Bitboard bb) {
    int count = 0;
    while (bb) {
        bb &= bb - 1; // Clear the least significant bit set
        ++count;
    }
    return count;
}