#pragma once
#include <array>
#include <random>
#include "attacks.h"
#include "types.h"
#include <cassert>
#include <iostream>

// Using "plain" magics, may switch to fancy magics later
struct Magic {
    Bitboard mask;
    Bitboard magic;
    int shift_amt;
};

void init_magics();
std::array<Magic, square_num> find_magics();
Bitboard get_bishop_attacks(Square sq, Bitboard blockers);
Bitboard get_rook_attacks(Square sq, Bitboard blockers);