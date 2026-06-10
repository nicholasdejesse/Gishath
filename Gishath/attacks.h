#pragma once
#include <array>
#include "types.h"
#include "bitboard.h"

constexpr Bitboard pawn_attacks(Color c, Bitboard sq) {
    Bitboard attack = 0;

    attack |= shift_BB(sq, relative_direction(c, north_east));
    attack |= shift_BB(sq, relative_direction(c, north_west));

    return attack;
}

constexpr Bitboard knights_attacks(Bitboard sq) {
    Bitboard attack = 0;

    Bitboard left = shift_BB(sq, west);
    Bitboard right = shift_BB(sq, east);
    Bitboard leftleft = shift_BB(left, west);
    Bitboard rightright = shift_BB(right, east);
    Bitboard up = shift_BB(sq, north);
    Bitboard down = shift_BB(sq, south);
    Bitboard upup = shift_BB(up, north);
    Bitboard downdown = shift_BB(down, south);

    attack |= shift_BB(leftleft, north);
    attack |= shift_BB(leftleft, south);
    attack |= shift_BB(rightright, north);
    attack |= shift_BB(rightright, south);
    attack |= shift_BB(upup, east);
    attack |= shift_BB(upup, west);
    attack |= shift_BB(downdown, east);
    attack |= shift_BB(downdown, west);

    return attack;
}

constexpr Bitboard king_attacks(Bitboard sq) {
    Bitboard attack = 0;
    for (Direction d : {north, east, south, west, north_east, north_west, south_east, south_west}) {
        attack |= shift_BB(sq, Direction(d));
    }
    return attack;
}

constexpr Bitboard sliding_attacks(Square sq, PieceType piece_type, Bitboard occ) {
    std::array<Direction, 4> directions = (piece_type == bishop) ? std::array{ north_east, north_west, south_east, south_west } : std::array{ north, east, south, west };
    
    Bitboard attack = 0;

    for (Direction d : directions) {
        Bitboard ray = shift_BB(square_to_bitboard(sq), d);
        while (ray & ~occ) {
            attack |= ray;
            ray = shift_BB(ray, d);
        }
    }

    return attack;
}


// Get attacks on an empty board
constexpr auto pseudo_attacks = []() constexpr {
    std::array<std::array<Bitboard, square_num>, piece_type_num> attacks{};

    for (int i = 0; i < square_num; ++i) {
        // Weird casting from color to piece type, but it works
        attacks[white][i] = pawn_attacks(white, square_to_bitboard(Square(i)));
        attacks[black][i] = pawn_attacks(black, square_to_bitboard(Square(i)));
        attacks[knight][i] = knights_attacks(square_to_bitboard(Square(i)));
        attacks[king][i] = king_attacks(square_to_bitboard(Square(i)));
        attacks[bishop][i] = sliding_attacks(Square(i), bishop, 0);
        attacks[rook][i] = sliding_attacks(Square(i), rook, 0);
        attacks[queen][i] = attacks[bishop][i] | attacks[rook][i];
    }

    return attacks;
}();