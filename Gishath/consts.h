#pragma once
#include <limits>
#include <array>

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
constexpr unsigned long long soft_node_hard_limit_multiplier = 4;
constexpr int reverse_futility_pruning_factor = 150;
constexpr int max_futility_pruning_depth = 3;
constexpr std::array<int, max_futility_pruning_depth> futility_pruning_factors{ 125, 500, 900 };
constexpr int killers_per_ply = 2;
constexpr int lmr_min_depth = 3;
constexpr int lmp_max_depth = 4;
constexpr int max_history_bonus = 2048;
constexpr int iir_min_depth = 6;
constexpr int iir_depth_reduction = 2;
constexpr int aspiration_window = 40; // Centipawns in one direction, full window is 2 times this
constexpr int aspiration_min_depth = 4;
constexpr int aspiration_max_tries = 4;
constexpr int delta_pruning = 350;

// Transposition table
constexpr int transposition_table_bucket_size = 4; // Number of buckets per index in transposition table
constexpr int transposition_table_age_cutoff = 1; // Number of moves until an entry is always replaced

// Eval (material and piece square stored in eval.h)
//constexpr int eval_bishop_pair = 40;
//constexpr int eval_doubled_pawn_penalty = 20;
//constexpr int eval_castle_missing_shield_pawn_penalty = 10;
//constexpr int eval_rook_open_file_bonus = 12;
//constexpr int eval_rook_semi_open_file_bonus = 6;
//constexpr int eval_passed_pawn_bonus = 20;
//constexpr int eval_isolated_pawn_penalty = 10;
constexpr int eval_mopup_king_on_edge_bonus = 300;
constexpr int eval_mopup_dist_between_king_bonus = 200;
//constexpr int eval_knight_mobility_bonus = 3;
//constexpr int eval_bishop_mobility_bonus = 2;

// UCI options
constexpr int uci_hash_min_size = 1;
constexpr int uci_hash_max_size = 512;
constexpr int uci_hash_default_size = 16;
