#include <chrono>
#include <utility>
#include "search.h"
#include "consts.h"
#include "uci.h"
#include "move_gen.h"
#include "eval.h"
#include "move_order.h"

int nodes_visited = 0;
bool out_of_time = false;
std::chrono::time_point<std::chrono::steady_clock> end_time;

void check_out_of_time() {
    auto time_now = std::chrono::steady_clock::now();
    if (time_now >= end_time) {
        out_of_time = true;
    }
}

int quiescence(Position& pos, int depth, int alpha, int beta, TranspositionTable& transposition_table) {
    ++nodes_visited;
    if (nodes_visited % out_of_time_check_freq == 0) {
        check_out_of_time();
    }
    if (pos.threefold_or_50_move())
        return draw_score;
    if (out_of_time)
        return draw_score; // Doesn't matter what we return here

    if (transposition_table.contains(pos.zobrist)) {
        TranspositionEntry tt_entry = transposition_table.get(pos.zobrist);
        if (tt_entry.depth >= depth && pos.pseudo_legal(tt_entry.best_move)) {
            switch (tt_entry.flag) {
            case TranspositionFlag::exact:
                return tt_entry.score;
            case TranspositionFlag::upper_bound:
                beta = std::min(beta, tt_entry.score);
                break;
            case TranspositionFlag::lower_bound:
                alpha = std::max(alpha, tt_entry.score);
                break;
            }
            if (alpha >= beta) return tt_entry.score;
        }
    }

    int max = evaluate_position(pos, depth);
    if (max >= beta)
        return max;
    if (max > alpha)
        alpha = max;

    MoveList ml = generate_by_type<GenType::captures>(pos);
    MoveOrder order = MoveOrder(ml, pos, transposition_table);
    if (ml.count == 0) return max;

    bool found_legal = false;
    Move best_move = Move();
    TranspositionFlag flag = TranspositionFlag::upper_bound;
    for (int i = 0; i < ml.count; ++i) {
        Move m = order.get_next_move();
        pos.make_move(m);
        if (!pos.legal()) {
            pos.unmake_move(m);
            continue;
        }
        found_legal = true;
        int score = -1 * quiescence(pos, depth - 1, -beta, -alpha, transposition_table);
        if (score > max) {
            max = score;
            best_move = m;
            if (score > alpha) {
                alpha = score;
            }
        }
        pos.unmake_move(m);
        if (score >= beta) {
            transposition_table.insert(TranspositionEntry(pos.zobrist, best_move, max, depth, TranspositionFlag::lower_bound));
            return max;
        }
    }
    if (!found_legal) return evaluate_position(pos, depth);

    transposition_table.insert(TranspositionEntry(pos.zobrist, best_move, max, depth, flag));

    return max;

}

int negamax_recursive(Position& pos, int depth, int alpha, int beta, TranspositionTable& transposition_table) {
    ++nodes_visited;

    // Call every so often since it is expensive
    if (nodes_visited % out_of_time_check_freq == 0) {
        check_out_of_time();
    }
    if (out_of_time)
        return draw_score; // Doesn't matter what we return here

    if (pos.threefold_or_50_move())
        return draw_score;

    if (transposition_table.contains(pos.zobrist)) {
        TranspositionEntry tt_entry = transposition_table.get(pos.zobrist);
        if (tt_entry.depth >= depth && pos.pseudo_legal(tt_entry.best_move)) {
            switch (tt_entry.flag) {
            case TranspositionFlag::exact:
                return tt_entry.score;
            case TranspositionFlag::upper_bound:
                beta = std::min(beta, tt_entry.score);
                break;
            case TranspositionFlag::lower_bound:
                alpha = std::max(alpha, tt_entry.score);
                break;
            }
            if (alpha >= beta) return tt_entry.score;
        }
    }

    if (depth <= 0) return quiescence(pos, depth, alpha, beta, transposition_table); // Don't flip alpha and beta because we aren't going down search tree this call

    Color us = pos.side_to_move();
    // Null move pruning
    // Don't allow when in check or side to move is only king and pawns
    if (depth >= null_move_reduction && !pos.is_checked(us) && !(pos.color_BB[us] & ~pos.type_BB[pawn] & ~pos.type_BB[king])) {
        pos.make_nullmove();
        int null_score = -1 * negamax_recursive(pos, depth - null_move_reduction, -beta, -beta + 1, transposition_table);
        pos.unmake_nullmove();
        if (null_score >= beta) {
            return null_score;
        }
    }

    MoveList ml = generate(pos);
    MoveOrder order = MoveOrder(ml, pos, transposition_table);
    if (ml.count == 0) return evaluate_position(pos, depth);

    bool found_legal = false;
    int max = negative_inf;
    Move best_move = Move();
    TranspositionFlag flag = TranspositionFlag::upper_bound;
    for (int i = 0; i < ml.count; ++i) {
        Move m = order.get_next_move();
        pos.make_move(m);
        if (!pos.legal()) {
            pos.unmake_move(m);
            continue;
        }
        found_legal = true;
        int score = -1 * negamax_recursive(pos, depth - 1, -beta, -alpha, transposition_table);
        if (score > max) {
            max = score;
            best_move = m;
            if (score > alpha) {
                flag = TranspositionFlag::exact;
                alpha = score;
            }
        }
        pos.unmake_move(m);
        if (score >= beta) {
            transposition_table.insert(TranspositionEntry(pos.zobrist, best_move, max, depth, TranspositionFlag::lower_bound));
            return max;
        }
    }
    if (!found_legal) return evaluate_position(pos, depth);

    transposition_table.insert(TranspositionEntry(pos.zobrist, best_move, max, depth, flag));

    return max;
}

std::pair<Move, int> negamax(Position& pos, int depth, TranspositionTable& transposition_table) {
    MoveList ml = generate(pos);
    MoveOrder order = MoveOrder(ml, pos, transposition_table);

    assert(ml.count > 0);


    int alpha = negative_inf;
    int beta = positive_inf;
    int max = negative_inf;
    Move best_move;
    TranspositionFlag flag = TranspositionFlag::upper_bound;
    
    for (int i = 0; i < ml.count; ++i) {
        Move m = order.get_next_move();
        //Move m = ml.moves[i];
        pos.make_move(m);
        if (!pos.legal()) {
            pos.unmake_move(m);
            continue;
        }
        int score = -1 * negamax_recursive(pos, depth - 1, -beta, -alpha, transposition_table);
        if (score > max) {
            max = score;
            best_move = m;
            if (score > alpha) {
                flag = TranspositionFlag::exact;
                alpha = score;
            }
        }
        pos.unmake_move(m);
    }

    transposition_table.insert(TranspositionEntry(pos.zobrist, best_move, max, depth, flag));

    return std::make_pair(best_move, max);
}

Move find_best_move(Position& pos, int ms_to_search, UCIEngine& uci, int max_depth = 9999) {
    nodes_visited = 0;
    out_of_time = false;
    end_time = std::chrono::steady_clock::now() + std::chrono::milliseconds(ms_to_search);

    int depth = 1;
    Move best_move;
    int best_score = negative_inf;
    while (!out_of_time && depth <= max_depth) {
        std::pair<Move, int> curr = negamax(pos, depth, uci.transposition_table);
        if (!out_of_time) {
            best_move = curr.first;
            best_score = curr.second;
        }
        uci.write_search_info(depth, best_score, nodes_visited);
        ++depth;
    }
    
    return best_move;
}