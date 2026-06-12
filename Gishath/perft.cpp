#include "perft.h"

int perft_recursive(Position& pos, int depth) {
    MoveList move_list{};
    int nodes = 0;

    generate(pos, move_list);
    if (depth == 0) return 1;
    //if (depth == 1) return move_list.count; // Bulk counting, faster result but not informative

    for (int i = 0; i < move_list.count; i++) {
        pos.make_move(move_list.moves[i]);
        if (pos.legal()) {
            nodes += perft_recursive(pos, depth - 1);
        }
        pos.unmake_move(move_list.moves[i]);
    }
    return nodes;
}

void run_perft(Position& pos, int depth) {
    auto start_time = std::chrono::steady_clock::now();
    MoveList move_list{};
    int total_nodes = 0;
    generate(pos, move_list);
    for (int i = 0; i < move_list.count; i++) {
        pos.make_move(move_list.moves[i]);
        if (pos.legal()) {
            int res = perft_recursive(pos, depth - 1);
            std::cout << move_list.moves[i].to_string() << ": " << res << "\n";
            total_nodes += res;
        }
        pos.unmake_move(move_list.moves[i]);
    }
    auto end_time = std::chrono::steady_clock::now();
    double elapsed_sec = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count() / 1000.0;
    double million_nodes_per_second = (total_nodes / elapsed_sec) / 1000000.0;

    std::cout << "\nNodes searched: " << total_nodes << "\n";
    std::cout << "Finished searching in " << elapsed_sec << " seconds (" << million_nodes_per_second << " million nodes/sec)\n";
}