#pragma once
#include <array>
#include "uci.h"
#include "move_gen.h"
#include "transposition_table.h"

class MoveOrder {
private:
    MoveList ml;
    std::array<int, move_list_size> scores;
    Position pos;
    int idx_sorted;
    int score_move(Move m, Position& pos, TranspositionEntry* tt_entry, int ply, UCIEngine& uci, bool quiescence, Move prev_move);

public:
    MoveOrder(MoveList& ml, Position& pos, int ply, UCIEngine& uci, bool quiescence, Move prev_move);
    Move get_next_move();
};