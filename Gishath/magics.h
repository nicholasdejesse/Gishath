#pragma once
#include "types.h"

// Using "plain" magics, may switch to fancy magics later
struct Magic {
    Bitboard mask;
    Bitboard magic;
    int shift_amt;
};

void init_magics();
Bitboard get_bishop_attacks_raw(Square sq, Bitboard blockers);
Bitboard get_rook_attacks_raw(Square sq, Bitboard blockers);