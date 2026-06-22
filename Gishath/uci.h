#pragma once
#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include "position.h"
#include "search.h"
#include "transposition_table.h"

const std::string start_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
constexpr int transposition_table_size = 2097152; // Make this an option in command line later

class UCIEngine {
private:
    Position pos;

    void new_game();
    void reset_position();
    int get_time_for_move(int wtime, int btime);

public:
    TranspositionTable transposition_table;
    void loop();
    void write_search_info(int depth, int score, int nodes_visited);

    UCIEngine();
};

