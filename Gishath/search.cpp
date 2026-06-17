#include "search.h"

int negamax_recursive(Position& pos, int depth) {
    if (depth == 0) return evaluate_position(pos);

    MoveList ml = generate(pos);
    if (ml.count == 0) return evaluate_position(pos);

    bool found_legal = false;
    int max = min_eval_score;
    for (int i = 0; i < ml.count; ++i) {
        pos.make_move(ml.moves[i]);
        if (!pos.legal()) {
            pos.unmake_move(ml.moves[i]);
            continue;
        }
        found_legal = true;
        int score = -1 * negamax_recursive(pos, depth - 1);
        if (score > max) max = score;
        pos.unmake_move(ml.moves[i]);
    }
    if (!found_legal) return evaluate_position(pos);
    return max;
}

Move negamax(Position& pos, int depth) {
    MoveList ml = generate(pos);
    if (ml.count == 0) return evaluate_position(pos);

    bool found_legal = false;
    int max = std::numeric_limits<int>::min();
    Move best_move;
    for (int i = 0; i < ml.count; ++i) {
        pos.make_move(ml.moves[i]);
        if (!pos.legal()) {
            pos.unmake_move(ml.moves[i]);
            continue;
        }
        found_legal = true;
        int score = -1 * negamax_recursive(pos, depth - 1);
        if (score > max) {
            max = score;
            best_move = ml.moves[i];
        }
        pos.unmake_move(ml.moves[i]);
    }
    if (!found_legal) return evaluate_position(pos);
    return best_move;
}

Move find_best_move(Position& pos) {
    return negamax(pos, 4);
}
