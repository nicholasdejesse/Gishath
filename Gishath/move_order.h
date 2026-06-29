#pragma once
#include <array>
#include "move_gen.h"
#include "transposition_table.h"

class MoveOrder {
private:
    MoveList ml;
    std::array<int, move_list_size> scores;
    Position pos;
    int idx_sorted;
    int score_move(Move m, Position& pos, TranspositionTable& transposition_table, std::array<std::array<Move, killers_per_ply>, max_ply>& killers, int ply);

public:
    MoveOrder(MoveList& ml, Position& pos, TranspositionTable& transposition_table, std::array<std::array<Move, killers_per_ply>, max_ply>& killers, int ply);
    Move get_next_move();
};