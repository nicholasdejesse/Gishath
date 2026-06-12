#pragma once
#include "move.h"
#include "position.h"
#include "bitboard.h"
#include "types.h"
#include "attacks.h"
#include <array>

struct MoveList {
    std::array<Move, move_list_size> moves;
    int count = 0;

    void push(Move m) {
        moves[count++] = m;
    }

    MoveList() : moves{}, count(0) {}
};

void generate(Position& pos, MoveList& ml);