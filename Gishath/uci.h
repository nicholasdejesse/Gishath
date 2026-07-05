#pragma once
#include <array>
#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include "position.h"
#include "search.h"
#include "transposition_table.h"

const std::string start_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

class UCIEngine {
private:
    Position pos;

    void new_game();
    void reset_position();
    int get_time_for_move(int wtime, int btime);

public:
    TranspositionTable transposition_table;
    std::array<std::array<Move, killers_per_ply>, max_ply> killers;
    std::array<std::array<std::array<int, square_num>, square_num>, color_num> history_table;

    void loop();
    void write_search_info(int depth, int seldepth, int score, int nodes_visited);

    UCIEngine();
};