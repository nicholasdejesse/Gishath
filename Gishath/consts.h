#pragma once
#include <limits>

// Constant values used for search

constexpr int max_ply = 256; // Max ply for search
constexpr int move_list_size = 256;

constexpr int checkmate_score = -999;
constexpr int draw_score = 0;
constexpr int negative_inf = std::numeric_limits<int>::min() / 2;
constexpr int positive_inf = std::numeric_limits<int>::max() / 2;

constexpr int null_move_reduction = 3;