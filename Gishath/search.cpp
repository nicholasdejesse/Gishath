#include <chrono>
#include <utility>
#include "search.h"
#include "consts.h"
#include "uci.h"
#include "movegen.h"
#include "eval.h"
#include "moveorder.h"

int nodes_visited = 0;
bool out_of_time = false;
std::chrono::time_point<std::chrono::steady_clock> end_time;

void check_out_of_time() {
    auto time_now = std::chrono::steady_clock::now();
    if (time_now >= end_time) {
        out_of_time = true;
    }
}

int negamax_recursive(Position& pos, int depth, int alpha, int beta) {
    ++nodes_visited;

    // Call every so often since it is expensive
    if (nodes_visited % out_of_time_check_freq == 0) {
        check_out_of_time();
    }
    if (out_of_time) {
        return draw_score;
    }
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
    MoveOrder order = MoveOrder(ml, pos);
    if (ml.count == 0) return evaluate_position(pos);

    bool found_legal = false;
    int max = negative_inf;
    for (int i = 0; i < ml.count; ++i) {
        Move m = order.get_next_move();
        pos.make_move(m);
        if (!pos.legal()) {
            pos.unmake_move(m);
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
        pos.unmake_move(m);
        if (score >= beta) {
            return max;
        }
    }
    if (!found_legal) return evaluate_position(pos);
    return max;
}

std::pair<Move, int> negamax(Position& pos, int depth) {
    MoveList ml = generate(pos);
    assert(ml.count > 0);

    int alpha = negative_inf;
    int beta = positive_inf;
    int max = negative_inf;
    Move best_move;
    for (int i = 0; i < ml.count; ++i) {
        pos.make_move(ml.moves[i]);
        if (!pos.legal()) {
            pos.unmake_move(ml.moves[i]);
            continue;
        }
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
    return std::make_pair(best_move, max);
}

Move find_best_move(Position& pos, int ms_to_search, UCIEngine& uci) {
    nodes_visited = 0;
    out_of_time = false;
    end_time = std::chrono::steady_clock::now() + std::chrono::milliseconds(ms_to_search);

    int depth = 1;
    Move best_move;
    int best_score = negative_inf;
    while (!out_of_time) {
        std::pair<Move, int> curr = negamax(pos, depth);
        if (!out_of_time) {
            best_move = curr.first;
            best_score = curr.second;
        }
        uci.write_search_info(depth, best_score, nodes_visited);
        ++depth;
    }
    
    return best_move;
}