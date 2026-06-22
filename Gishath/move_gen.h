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

// Types of moves to generate
enum class GenType {
    captures,
    quiet,
    all
};

struct MoveList {
    std::array<Move, move_list_size> moves;
    int count = 0;

    void push(Move m);

    MoveList();
};

template <GenType type>
MoveList generate_by_type(Position& pos);

MoveList generate(Position& pos);