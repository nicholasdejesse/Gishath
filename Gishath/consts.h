#pragma once
#include <limits>

constexpr int max_ply = 256; // Max ply for search
constexpr int move_list_size = 256;
constexpr int initial_history_length = 512;

constexpr int checkmate_score = 30000;
constexpr int draw_score = 0;
constexpr int negative_inf = std::numeric_limits<int>::min() / 2;
constexpr int positive_inf = std::numeric_limits<int>::max() / 2;

// Search
constexpr int null_move_reduction = 2;
constexpr int out_of_time_check_freq = 2048; // How many nodes to visit before rechecking if we are out of time
constexpr int reverse_futility_pruning_factor = 150;
constexpr int futility_pruning_factor = 250;
constexpr int killers_per_ply = 2;
constexpr int lmr_min_depth = 3;
constexpr int max_history_bonus = 2048;

// Transposition table
constexpr int transposition_table_size = 8388608; // Make this an option in command line later
constexpr int transposition_table_bucket_size = 4; // Number of buckets per index in transposition table
constexpr int transposition_table_age_cutoff = 1; // Number of moves until an entry is always replaced

// Eval (material and piece square stored in eval.h)
constexpr int eval_bishop_pair = 40;
constexpr int eval_doubled_pawn_penalty = 20;
constexpr int eval_castle_missing_shield_pawn_penalty = 10;
constexpr int eval_rook_open_file_bonus = 12;
constexpr int eval_rook_semi_open_file_bonus = 6;
constexpr int eval_passed_pawn_bonus = 20;
//constexpr int eval_knight_mobility_bonus = 3;
//constexpr int eval_bishop_mobility_bonus = 2;
