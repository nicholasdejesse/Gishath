#pragma once
#include "position.h"
#include "move.h"
#include "uci.h"

class UCIEngine;


Move find_best_move(Position& pos, unsigned long ms_to_search, UCIEngine& uci, int max_depth);