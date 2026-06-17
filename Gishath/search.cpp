#include "search.h"
#include "consts.h"

int negamax_recursive(Position& pos, int depth, int alpha, int beta) {
    if (depth <= 0) return evaluate_position(pos);

    Color us = pos.side_to_move();
    // Null move pruning
    // Don't allow when in check or side to move is only king and pawns
    if (depth >= null_move_reduction && !pos.is_checked(us) && !(pos.color_BB[us] & ~pos.type_BB[pawn] & ~pos.type_BB[king])) {
        pos.make_nullmove();
        int null_score = -1 * negamax_recursive(pos, depth - null_move_reduction, -beta, -beta + 1);
        pos.unmake_nullmove();
        if (null_score >= beta) {
            return null_score;
        }
    }

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
        int score = -1 * negamax_recursive(pos, depth - 1, -beta, -alpha);
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
