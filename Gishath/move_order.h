#pragma once
#include <array>
#include "move_gen.h"

class MoveOrder {
private:
    MoveList ml;
    std::array<int, move_list_size> scores;
    Position pos;
    int idx_sorted;
    int score_move(Move m);

public:
    MoveOrder(MoveList& ml, Position& pos);
    Move get_next_move();
};