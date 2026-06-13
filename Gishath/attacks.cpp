#include "attacks.h"

Bitboard get_attacks(Color us, PieceType pt, Square from, Bitboard occ) {
    switch (pt) {
    case pawn:
        return pawn_attacks(us, square_to_bitboard(from));
    case knight:
        return knights_attacks(from);
    case bishop:
        return get_bishop_attacks_raw(from, occ);
    case rook:
        return get_rook_attacks_raw(from, occ);
    case queen:
        return get_bishop_attacks_raw(from, occ) | get_rook_attacks_raw(from, occ);
    case king:
        return king_attacks(square_to_bitboard(from));
    }
    return Bitboard(0);
}

Bitboard get_attacks(Color us, PieceType pt, Bitboard from, Bitboard occ) {
    Bitboard res = 0;
    switch (pt) {
    case pawn:
        return pawn_attacks(us, from);
    case knight:
        while (from) {
            res |= knights_attacks(pop_lsb(from));
        }
        return res;
    case bishop:
        while (from) {
            res |= get_bishop_attacks_raw((pop_lsb(from)), occ);
        }
        return res;
    case rook:
        while (from) {
            res |= get_rook_attacks_raw((pop_lsb(from)), occ);
        }
        return res;
    case queen:
        while (from) {
            Square sq = pop_lsb(from);
            res |= get_bishop_attacks_raw((sq), occ);
            res |= get_rook_attacks_raw((sq), occ);
        }
        return res;
    case king:
        return king_attacks(from);
    default:
        return 0;
    }
}