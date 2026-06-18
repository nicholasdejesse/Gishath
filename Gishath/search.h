#pragma once
#include <limits>
#include "position.h"
#include "move.h"
#include "movegen.h"
#include "eval.h"

Move find_best_move(Position& pos, int ms_to_search);