#pragma once
#include <array>
#include <cmath>
#include "magics.h"
#include "types.h"
#include "bitboard.h"

constexpr Bitboard pawn_attacks(Color us, Bitboard bb) {
    Bitboard attack = 0;

    attack |= shift_BB(bb, relative_direction(us, north_east));
    attack |= shift_BB(bb, relative_direction(us, north_west));

    return attack;
}

constexpr Bitboard knights_attacks(Square sq) {
    Bitboard attack = 0;

    for (int move : {-17, -15, -10, -6, 6, 10, 15, 17}) {
        Square target = sq + move;
        Bitboard res = target >= 0 && target < square_num && std::abs(file_of(target) - file_of(sq)) <= 2 ? square_to_bitboard(target) : Bitboard(0);
        attack |= res;
    }

    return attack;
}

constexpr Bitboard king_attacks(Bitboard bb) {
    Bitboard attack = 0;
    for (Direction d : {north, east, south, west, north_east, north_west, south_east, south_west}) {
        attack |= shift_BB(bb, Direction(d));
    }
    return attack;
}

// Gives attacks for sliding pieces assuming the board is empty
constexpr Bitboard sliding_attacks(Square bb, PieceType pt, Bitboard occ) {
    constexpr auto bishop_directions = std::array{ north_east, north_west, south_east, south_west };
    constexpr auto rook_directions = std::array{ north, east, south, west };
    
    Bitboard attack = 0;

    for (Direction d : pt == bishop ? bishop_directions : rook_directions) {
        Bitboard ray = shift_BB(square_to_bitboard(bb), d);
        while (ray) {
            attack |= ray;
            if (ray & occ) break;
            ray = shift_BB(ray, d);
        }
    }

    return attack;
}

Bitboard get_attacks(Color us, PieceType pt, Square from, Bitboard occ);
Bitboard get_attacks(Color us, PieceType pt, Bitboard from, Bitboard occ);

// Get attacks on an empty board
//constexpr auto pseudo_attacks = []() constexpr {
//    std::array<std::array<Bitboard, square_num>, piece_type_num> attacks{};
//
//    for (int i = 0; i < square_num; ++i) {
//        // Weird casting from color to piece type, but it works
//        attacks[white][i] = pawn_attacks(white, square_to_bitboard(Square(i)));
//        attacks[black][i] = pawn_attacks(black, square_to_bitboard(Square(i)));
//        attacks[knight][i] = knights_attacks(square_to_bitboard(Square(i)));
//        attacks[king][i] = king_attacks(square_to_bitboard(Square(i)));
//        attacks[bishop][i] = sliding_attacks(Square(i), bishop, 0);
//        attacks[rook][i] = sliding_attacks(Square(i), rook, 0);
//        attacks[queen][i] = attacks[bishop][i] | attacks[rook][i];
//    }
//
//    return attacks;
//}();
//
