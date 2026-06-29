#include <chrono>
#include <utility>
#include <algorithm>
#include "search.h"
#include "consts.h"
#include "uci.h"
#include "move_gen.h"
#include "eval.h"
#include "move_order.h"

Bitboard prev_zobrist = 0;
uint8_t age = 0;
int nodes_visited = 0;
int seldepth = 0;
bool out_of_time = false;
std::chrono::time_point<std::chrono::steady_clock> end_time;

int16_t adjust_score_for_tt(int score, int ply) {
    if (score >= checkmate_score - max_ply)  return score + ply;
    if (score <= -checkmate_score + max_ply) return score - ply;
    return score;
}

int16_t get_ply_adjusted_mate_score(int score, int ply) {
    if (score >= checkmate_score - max_ply) return score - ply;
    if (score <= -checkmate_score + max_ply) return score + ply;
    return score;
}

void check_out_of_time() {
    auto time_now = std::chrono::steady_clock::now();
    if (time_now >= end_time) {
        out_of_time = true;
    }
}

int quiescence(Position& pos, int depth, int ply, int alpha, int beta, TranspositionTable& transposition_table) {
    ++nodes_visited;
    if (nodes_visited % out_of_time_check_freq == 0) {
        check_out_of_time();
    }
    if (pos.threefold_or_50_move() || pos.insufficient_material())
        return draw_score;
    if (out_of_time)
        return draw_score; // Doesn't matter what we return here

    TranspositionEntry* tt_entry = transposition_table.probe(pos.zobrist);
    if (tt_entry) {
        if (tt_entry->depth >= depth && pos.pseudo_legal(tt_entry->best_move)) {
            int depth_adjusted_score = get_ply_adjusted_mate_score(tt_entry->score, ply);
            switch (tt_entry->flag) {
            case TranspositionFlag::exact:
                return depth_adjusted_score;
            case TranspositionFlag::upper_bound:
                beta = std::min<int>(beta, depth_adjusted_score);
                break;
            case TranspositionFlag::lower_bound:
                alpha = std::max<int>(alpha, depth_adjusted_score);
                break;
            }
            if (alpha >= beta) return depth_adjusted_score;
        }
    }

    int max = evaluate_position(pos);
    if (max >= beta)
        return max;
    if (max > alpha)
        alpha = max;

    bool is_check = pos.is_checked(pos.side_to_move());
    MoveList ml = is_check ? generate(pos) : generate_by_type<GenType::captures>(pos); // This needs to switch to only generate evasions
    MoveOrder order = MoveOrder(ml, pos, transposition_table);

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
        int score = -1 * quiescence(pos, depth - 1, ply + 1, -beta, -alpha, transposition_table);
        if (score > max) {
            max = score;
            best_move = m;
            if (score > alpha) {
                alpha = score;
            }
        }
        pos.unmake_move(m);
        if (score >= beta) {
            transposition_table.insert(pos.zobrist, best_move, adjust_score_for_tt(max, ply), depth, TranspositionFlag::lower_bound, age);
            return max;
        }
    }
    if (!found_legal && pos.is_checked(pos.side_to_move())) {
        return -(checkmate_score - ply);
    }

    transposition_table.insert(pos.zobrist, best_move, adjust_score_for_tt(max, ply), depth, flag, age);
    if (depth < seldepth) seldepth = depth;
    return max;

}

int pvs_recursive(Position& pos, int depth, int ply, int alpha, int beta, TranspositionTable& transposition_table) {
    ++nodes_visited;

    // Call every so often since it is expensive
    if (nodes_visited % out_of_time_check_freq == 0) {
        check_out_of_time();
    }
    if (out_of_time)
        return draw_score; // Doesn't matter what we return here

    if (pos.threefold_or_50_move() || pos.insufficient_material())
        return draw_score;

    TranspositionEntry* tt_entry = transposition_table.probe(pos.zobrist);
    if (tt_entry) {
        if (tt_entry->depth >= depth && pos.pseudo_legal(tt_entry->best_move)) {
            int depth_adjusted_score = get_ply_adjusted_mate_score(tt_entry->score, ply);
            switch (tt_entry->flag) {
            case TranspositionFlag::exact:
                return depth_adjusted_score;
            case TranspositionFlag::upper_bound:
                beta = std::min<int>(beta, depth_adjusted_score);
                break;
            case TranspositionFlag::lower_bound:
                alpha = std::max<int>(alpha, depth_adjusted_score);
                break;
            }
            if (alpha >= beta) return depth_adjusted_score;
        }
    }

    if (depth <= 0) return quiescence(pos, depth, ply, alpha, beta, transposition_table); // Don't flip alpha and beta because we aren't going down search tree this call

    Color us = pos.side_to_move();
    bool is_checked = pos.is_checked(us);
    // Null move pruning
    // Don't allow when in check or side to move is only king and pawns (zugzwang)
    if (depth >= null_move_reduction && !pos.is_checked(us) && (pos.color_BB[us] & ~pos.type_BB[pawn] & ~pos.type_BB[king])) {
        pos.make_nullmove();
        int null_score = -1 * pvs_recursive(pos, depth - null_move_reduction, ply + 1, -beta, -beta + 1, transposition_table);
        pos.unmake_nullmove();
        if (null_score >= beta) {
            transposition_table.insert(pos.zobrist, Move(0), adjust_score_for_tt(null_score, ply), depth, TranspositionFlag::lower_bound, age);
            return null_score;
        }
    }

    // Reverse futility pruning
    int eval = evaluate_position(pos);
    int margin = reverse_futility_pruning_factor * depth;
    // Not in check, not PV node, not checkmate
    if (!is_checked && beta - alpha <= 1 && eval >= beta + margin && std::abs(beta) < checkmate_score - max_ply) {
        return eval;
    }

    MoveList ml = generate(pos);
    MoveOrder order = MoveOrder(ml, pos, transposition_table);

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
        int score;
        if (i == 0) {
            score = -1 * pvs_recursive(pos, depth - 1, ply + 1, -beta, -alpha, transposition_table);
        }
        else {
            score = -1 * pvs_recursive(pos, depth - 1, ply + 1, -alpha - 1, -alpha, transposition_table);
            if (score > alpha && beta - alpha > 1)
                score = -1 * pvs_recursive(pos, depth - 1, ply + 1, -beta, -alpha, transposition_table);
        }
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
            transposition_table.insert(pos.zobrist, best_move, adjust_score_for_tt(max, ply), depth, TranspositionFlag::lower_bound, age);
            return max;
        }
    }
    if (!found_legal) {
        if (is_checked) {
            return -(checkmate_score - ply);
        }
        else {
            return draw_score;
        }
    }

    transposition_table.insert(pos.zobrist, best_move, adjust_score_for_tt(max, ply), depth, flag, age);
    if (depth < seldepth) seldepth = depth;
    return max;
}

std::pair<Move, int> pvs(Position& pos, int depth, TranspositionTable& transposition_table) {
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
        int score;
        if (i == 0) {
            score = -1 * pvs_recursive(pos, depth - 1, 1,  -beta, -alpha, transposition_table);
        }
        else {
            score = -1 * pvs_recursive(pos, depth - 1, 1, -alpha - 1, -alpha, transposition_table);
            if (score > alpha && beta - alpha > 1)
                score = -1 * pvs_recursive(pos, depth - 1, 1, -beta, -alpha, transposition_table);
        }
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

    transposition_table.insert(pos.zobrist, best_move, adjust_score_for_tt(max, 0), depth, flag, age);

    return std::make_pair(best_move, max);
}

Move find_best_move(Position& pos, int ms_to_search, UCIEngine& uci, int max_depth = 9999) {
    // Prevent age increase from searching same position twice
    // (Won't happen in engine play but maybe analysis idk)
    if (pos.zobrist != prev_zobrist) {
        ++age;
        prev_zobrist = pos.zobrist;
    }
    nodes_visited = 0;
    seldepth = 0;
    out_of_time = false;
    end_time = std::chrono::steady_clock::now() + std::chrono::milliseconds(ms_to_search);

    int depth = 1;
    Move best_move;
    int best_score = negative_inf;
    while (!out_of_time && depth <= max_depth) {
        std::pair<Move, int> curr = pvs(pos, depth, uci.transposition_table);
        if (!out_of_time) {
            best_move = curr.first;
            best_score = curr.second;
            uci.write_search_info(depth, std::max(depth, depth - seldepth), best_score, nodes_visited);
            ++depth;
            if (best_score > checkmate_score - max_ply) break;
        }
    }

    return best_move;
}