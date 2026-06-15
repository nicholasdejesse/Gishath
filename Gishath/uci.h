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

public:
    void loop();
};

