#include "perft.h"

uint64_t perft_recursive(Position& pos, int depth) {
    uint64_t nodes = 0;

    if (depth == 0) return 1;
    MoveList ml = generate(pos);

    for (int i = 0; i < ml.count; i++) {
        pos.make_move(ml.moves[i]);
        if (pos.legal()) {
            nodes += perft_recursive(pos, depth - 1);
        }
        pos.unmake_move(ml.moves[i]);
    }
    return nodes;
}

void run_perft(Position& pos, int depth) {
    auto start_time = std::chrono::steady_clock::now();
    MoveList ml = generate(pos);
    Bitboard total_nodes = 0;
    for (int i = 0; i < ml.count; i++) {
        pos.make_move(ml.moves[i]);
        if (pos.legal()) {
            uint64_t res = perft_recursive(pos, depth - 1);
            std::cout << ml.moves[i].to_string() << ": " << res << "\n";
            total_nodes += res;
        }
        pos.unmake_move(ml.moves[i]);
    }
    auto end_time = std::chrono::steady_clock::now();
    double elapsed_sec = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count() / 1000.0;
    double million_nodes_per_second = (total_nodes / elapsed_sec) / 1000000.0;

    std::cout << "\nNodes searched: " << total_nodes << "\n";
    std::cout << "Finished searching in " << elapsed_sec << " seconds (" << million_nodes_per_second << " million nodes/sec)\n";
}