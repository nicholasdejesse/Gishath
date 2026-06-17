#pragma once
#include "move.h"
#include "position.h"
#include "bitboard.h"
#include "types.h"
#include "attacks.h"
#include <array>
#include <cassert>
#include "consts.h"

class Position;

struct MoveList {
    std::array<Move, move_list_size> moves;
    int count = 0;

    void push(Move m);

    MoveList();
};

void generate(Position& pos, MoveList& ml);
MoveList generate(Position& pos);