#pragma once
#include <limits>

constexpr int max_ply = 256; // Max ply for search
constexpr int move_list_size = 256;
constexpr int initial_history_length = 512;

constexpr int checkmate_score = 30000;
constexpr int draw_score = 0;
constexpr int negative_inf = std::numeric_limits<int>::min() / 2;
constexpr int positive_inf = std::numeric_limits<int>::max() / 2;

constexpr int null_move_reduction = 3;
constexpr int out_of_time_check_freq = 2048; // How many nodes to visit before rechecking if we are out of time

// Transposition table
constexpr int transposition_table_size = 8388608; // Make this an option in command line later
constexpr int transposition_table_bucket_size = 2; // Number of buckets per index in transposition table