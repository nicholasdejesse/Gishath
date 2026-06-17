#include "search.h"

int negamax_recursive(Position& pos, int depth, int alpha, int beta) {
    if (depth == 0) return evaluate_position(pos);

    MoveList ml = generate(pos);
    if (ml.count == 0) return evaluate_position(pos);

    bool found_legal = false;
    int max = negative_inf;
    for (int i = 0; i < ml.count; ++i) {
        pos.make_move(ml.moves[i]);
        if (!pos.legal()) {
            pos.unmake_move(ml.moves[i]);
            continue;
        }
        found_legal = true;
        int score = -1 * negamax_recursive(pos, depth - 1, -beta, -alpha);
        if (score > max) {
            max = score;
            if (score > alpha) {
                alpha = score;
            }
        }
        pos.unmake_move(ml.moves[i]);
        if (score >= beta) {
            return max;
        }
    }
    if (!found_legal) return evaluate_position(pos);
    return max;
}

Move negamax(Position& pos, int depth) {
    MoveList ml = generate(pos);
    if (ml.count == 0) return evaluate_position(pos);

    int alpha = negative_inf;
    int beta = positive_inf;
    bool found_legal = false;
    int max = negative_inf;
    Move best_move;
    for (int i = 0; i < ml.count; ++i) {
        pos.make_move(ml.moves[i]);
        if (!pos.legal()) {
            pos.unmake_move(ml.moves[i]);
            continue;
        }
        found_legal = true;
        int score = -negamax_recursive(pos, depth - 1, -beta, -alpha);
        if (score > max) {
            max = score;
            best_move = ml.moves[i];
            if (score > alpha) {
                alpha = score;
            }
        }
        pos.unmake_move(ml.moves[i]);
    }
    if (!found_legal) return evaluate_position(pos);
    return best_move;
}

Move find_best_move(Position& pos) {
    return negamax(pos, 5);
}
