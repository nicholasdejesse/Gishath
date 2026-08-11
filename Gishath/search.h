#pragma once
#include "position.h"
#include "move.h"
#include "uci.h"
#include <limits>

class UCIEngine;


Move find_best_move(Position& pos, unsigned long ms_to_search, UCIEngine& uci, int max_depth = 9999, unsigned long long max_nodes = std::numeric_limits<unsigned long long>::max(), bool write_info = true);
std::pair<Move, int16_t> find_best_move_and_score(Position& pos, unsigned long ms_to_search, UCIEngine& uci, int max_depth = 9999, unsigned long long max_nodes = std::numeric_limits<unsigned long long>::max(), bool write_info = true);