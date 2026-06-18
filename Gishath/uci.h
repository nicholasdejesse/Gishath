#pragma once
#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include "position.h"
#include "search.h"

const std::string start_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

class UCIEngine {
private:
    Position pos;

    void new_game();
    void reset_position();
    int get_time_for_move(int wtime, int btime);

public:
    void loop();
    void write_search_info(int depth, int score, int nodes_visited);
};

