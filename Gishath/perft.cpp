#include "perft.h"

int perft_recursive(Position& pos, int depth) {
    MoveList move_list{};
    int nodes = 0;

    if (depth == 0)
        return 1;

    generate(pos, move_list);
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
    std::cout << "\nNodes searched: " << total_nodes;
}