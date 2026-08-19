#pragma once
#include <cstdlib>
#include "types.h"
#include <array>

constexpr Color get_square_color(Square sq) {
    return Color((Bitboard(0xAA55AA55AA55AA55) >> sq) & 1);
}

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
        pop_lsb(bb);
        ++count;
    }
    return count;
}
constexpr bool single_pop(Bitboard bb) {
    return bb != 0 && (bb & (bb - 1)) == 0;
}

constexpr int manhattan_distance(Square sq1, Square sq2) {
    int file1, file2, rank1, rank2;
    int rankDistance, fileDistance;
    file1 = sq1 & 7;
    file2 = sq2 & 7;
    rank1 = sq1 >> 3;
    rank2 = sq2 >> 3;
    rankDistance = std::abs(rank2 - rank1);
    fileDistance = std::abs(file2 - file1);
    return rankDistance + fileDistance;
}
