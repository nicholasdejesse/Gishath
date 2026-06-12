#include "attacks.h"

Bitboard get_attacks(Color us, PieceType pt, Square from, Bitboard occ) {
    switch (pt) {
    case pawn:
        return pawn_attacks(us, square_to_bitboard(from));
    case knight:
        return knights_attacks(square_to_bitboard(from));
    case bishop:
        return get_bishop_attacks_raw(from, occ);
    case rook:
        return get_rook_attacks_raw(from, occ);
    case queen:
        return get_bishop_attacks_raw(from, occ) | get_rook_attacks_raw(from, occ);
    case king:
        return king_attacks(square_to_bitboard(from));
    }
}