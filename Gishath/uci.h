#pragma once
#include <array>
#include <string>
#include <atomic>
#include <thread>
#include <iostream>
#include <sstream>
#include <vector>
#include "position.h"
#include "search.h"
#include "transposition_table.h"
#include "pv_table.h"


const std::string start_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

class UCIEngine {
private:

    unsigned long get_time_for_move(unsigned long wtime, unsigned long btime, unsigned long winc, unsigned long binc);
    std::thread search_thread;

public:
    Position pos;
    size_t hash_size = uci_hash_default_size;
    TranspositionTable transposition_table;
    std::array<std::array<Move, killers_per_ply>, max_ply> killers;
    std::array<std::array<std::array<int, square_num>, square_num>, color_num> history_table;
    std::array<std::array<Move, square_num>, square_num> countermove_table;
    PVTable pv_table;

    void new_game();
    void reset_position();
    void loop();
    void write_search_info(int depth, int seldepth, int score, unsigned long long nodes_visited, int time_elapsed_ms);

    std::atomic<bool> stop_search{ false };
    bool use_soft_nodes = false;

    UCIEngine();
};