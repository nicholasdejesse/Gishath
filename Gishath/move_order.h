#pragma once
#include <array>
#include "uci.h"
#include "move_gen.h"
#include "transposition_table.h"

class MoveOrder {
private:
    MoveList& ml;
    std::array<int, move_list_size> scores;
    int idx_sorted;
    int score_move(Move m, Position& pos, TranspositionEntry* tt_entry, int ply, UCIEngine& uci, bool quiescence, Move prev_move);

    MoveList losing_captures;
    int losing_capture_idx = 0;

public:
    MoveOrder(MoveList& ml, Position& pos, int ply, UCIEngine& uci, bool quiescence, Move prev_move);
    Move get_next_move();
    bool is_losing_capture(Move m, Position& pos);
    void push_losing_capture(Move m);
    bool should_search_losing_captures();
};
