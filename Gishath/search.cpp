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

void update_history(std::array<std::array<std::array<int, square_num>, square_num>, color_num>& history_table, Square from, Square to, Color us, int bonus) {
    int clamped_bonus = std::min(max_history_bonus, std::max(-max_history_bonus, bonus));
    history_table[us][from][to] += clamped_bonus - history_table[us][from][to] * std::abs(clamped_bonus) / max_history_bonus;
}

int16_t adjust_score_for_tt(int score, int ply) {
    if (score >= checkmate_score - max_ply)  return score + ply;
    if (score <= -checkmate_score + max_ply) return score - ply;
    return score;
}

int get_lmr_reduction(int depth, int move_number) {
    // Cast is fine since this is never called during quiescence search
    return static_cast<int>(std::min(4.0, std::round(0.3 + std::log(static_cast<unsigned int>(depth)) * std::log(static_cast<unsigned int>(move_number)) / 5)));
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

void insert_killer(std::array<std::array<Move, killers_per_ply>, max_ply>& killers, Position& pos, Move m, int ply) {
    if (pos.is_capture(m)) return;
    if (m != killers[ply][0]) {
        for (int i = killers_per_ply - 1; i > 0; --i) {
            killers[ply][i] = killers[ply][i - 1];
        }
        killers[ply][0] = m;
    }
}

int quiescence(Position& pos, int depth, int ply, int alpha, int beta, UCIEngine& uci) {
    ++nodes_visited;
    if (nodes_visited % out_of_time_check_freq == 0) {
        check_out_of_time();
    }
    if (pos.threefold_or_50_move() || pos.insufficient_material())
        return draw_score;
    if (out_of_time)
        return draw_score; // Doesn't matter what we return here

    TranspositionEntry* tt_entry = uci.transposition_table.probe(pos.zobrist);
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
    MoveOrder order = MoveOrder(ml, pos, ply, uci);

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
        int score = -1 * quiescence(pos, depth - 1, ply + 1, -beta, -alpha, uci);
        if (score > max) {
            max = score;
            best_move = m;
            if (score > alpha) {
                alpha = score;
            }
        }
        pos.unmake_move(m);
        if (score >= beta) {
            uci.transposition_table.insert(pos.zobrist, best_move, adjust_score_for_tt(max, ply), depth, TranspositionFlag::lower_bound, age);
            insert_killer(uci.killers, pos, m, ply);
            return max;
        }
    }
    if (!found_legal && pos.is_checked(pos.side_to_move())) {
        return -(checkmate_score - ply);
    }

    uci.transposition_table.insert(pos.zobrist, best_move, adjust_score_for_tt(max, ply), depth, flag, age);
    if (depth < seldepth) seldepth = depth;
    return max;

}

int pvs_recursive(Position& pos, int depth, int ply, int alpha, int beta, UCIEngine& uci) {
    ++nodes_visited;
    uci.pv_table.pv_length[ply] = 0;

    // Call every so often since it is expensive
    if (nodes_visited % out_of_time_check_freq == 0) {
        check_out_of_time();
        if (uci.stop_search.load()) {
            out_of_time = true;
        }
    }
    if (out_of_time)
        return draw_score; // Doesn't matter what we return here

    if (pos.threefold_or_50_move() || pos.insufficient_material())
        return draw_score;

    TranspositionEntry* tt_entry = uci.transposition_table.probe(pos.zobrist);
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

    if (depth <= 0) return quiescence(pos, depth, ply, alpha, beta, uci); // Don't flip alpha and beta because we aren't going down search tree this call

    Color us = pos.side_to_move();
    bool is_checked = pos.is_checked(us);
    if (is_checked) depth += 1;
    // Null move pruning
    // Don't allow when in check or side to move is only king and pawns (zugzwang)
    if (depth >= null_move_reduction && !pos.is_checked(us) && (pos.color_BB[us] & ~pos.type_BB[pawn] & ~pos.type_BB[king])) {
        pos.make_nullmove();
        int null_score = -1 * pvs_recursive(pos, depth - 1 - null_move_reduction, ply + 1, -beta, -beta + 1, uci);
        pos.unmake_nullmove();
        if (null_score >= beta) {
            uci.transposition_table.insert(pos.zobrist, Move(0), adjust_score_for_tt(null_score, ply), depth, TranspositionFlag::lower_bound, age);
            return null_score;
        }
    }

    // Reverse futility pruning
    int eval = evaluate_position(pos);
    int rfp_margin = reverse_futility_pruning_factor * depth;
    // Not in check, not PV node, not checkmate
    if (!is_checked && beta - alpha <= 1 && eval >= beta + rfp_margin && std::abs(beta) < checkmate_score - max_ply) {
        return eval;
    }

    MoveList ml = generate(pos);
    MoveOrder order = MoveOrder(ml, pos, ply, uci);

    bool found_legal = false;
    bool found_nonfutile = false;

    int max = negative_inf;
    Move best_move = Move();
    TranspositionFlag flag = TranspositionFlag::upper_bound;

    std::array<Move, move_list_size> quiets;
    int quiet_count = 0;

    for (int i = 0; i < ml.count; ++i) {
        Move m = order.get_next_move();
        bool is_capture = pos.is_capture(m);
        pos.make_move(m);
        if (!pos.legal()) {
            pos.unmake_move(m);
            continue;
        }

        found_legal = true;
        int iter_depth = depth;

        // Futility pruning
        if (depth <= max_futility_pruning_depth) {
            // Not capture, not checking, not PV node, not checkmate
            int futility = futility_pruning_factors[depth - 1]; // Subtract one because depth 1 corresponds to index 0 in array
            if (!is_capture &&
                !pos.is_checked(pos.side_to_move()) &&
                beta - alpha <= 1 &&
                eval + futility <= alpha &&
                alpha > -checkmate_score + max_ply &&
                beta < checkmate_score - max_ply)
            {
                if (depth == max_futility_pruning_depth) {
                    // Reduced depth search
                    --iter_depth;
                }
                else {
                    pos.unmake_move(m);
                    continue;
                }
            }
        }

        found_nonfutile = true;
        int score;
        if (i == 0) {
            score = -1 * pvs_recursive(pos, iter_depth - 1, ply + 1, -beta, -alpha, uci);
        }
        else {
            if (iter_depth > lmr_min_depth && !pos.is_checked(pos.side_to_move()) && !pos.is_capture(m)) {
                score = -1 * pvs_recursive(pos, iter_depth - 1 - get_lmr_reduction(iter_depth, i), ply + 1, -alpha - 1, -alpha, uci);
                if (score > alpha)
                    score = -1 * pvs_recursive(pos, iter_depth - 1, ply + 1, -alpha - 1, -alpha, uci);
            }
            else {
                score = -1 * pvs_recursive(pos, iter_depth - 1, ply + 1, -alpha - 1, -alpha, uci);
            }
            if (score > alpha && beta - alpha > 1)
                score = -1 * pvs_recursive(pos, iter_depth - 1, ply + 1, -beta, -alpha, uci);
        }
        if (score > max) {
            max = score;
            best_move = m;
            if (score > alpha) {
                flag = TranspositionFlag::exact;
                alpha = score;
                uci.pv_table.set_pv(m, ply);
            }
        }
        pos.unmake_move(m);
        if (score >= beta) {
            uci.transposition_table.insert(pos.zobrist, best_move, adjust_score_for_tt(max, ply), iter_depth, TranspositionFlag::lower_bound, age);
            insert_killer(uci.killers, pos, m, ply);
            if (!pos.is_capture(m)) {
                int update_val = iter_depth * iter_depth;
                update_history(uci.history_table, m.get_from(), m.get_to(), us, update_val);
                for (int j = 0; j < quiet_count; ++j) {
                    // Apply malus to other quiets that did not cause cutoff
                    update_history(uci.history_table, quiets[j].get_from(), quiets[j].get_to(), us, -update_val);
                }
            }
            return max;
        }
        if (!pos.is_capture(m)) {
            quiets[quiet_count++] = m;
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
    else if (!found_nonfutile) {
        return eval;
    }

    uci.transposition_table.insert(pos.zobrist, best_move, adjust_score_for_tt(max, ply), depth, flag, age);
    if (depth < seldepth) seldepth = depth;
    return max;
}

std::pair<Move, int> pvs(Position& pos, int depth, UCIEngine& uci) {
    MoveList ml = generate(pos);
    MoveOrder order = MoveOrder(ml, pos, 0, uci);
    uci.pv_table.pv_length[0] = 0;

    assert(ml.count > 0);

    int alpha = negative_inf;
    int beta = positive_inf;
    int max = negative_inf;
    Move best_move;
    TranspositionFlag flag = TranspositionFlag::upper_bound;
    
    for (int i = 0; i < ml.count; ++i) {
        if (out_of_time)
            return std::make_pair(Move(0), draw_score);
        Move m = order.get_next_move();
        //Move m = ml.moves[i];
        pos.make_move(m);
        if (!pos.legal()) {
            pos.unmake_move(m);
            continue;
        }
        int score;
        if (i == 0) {
            score = -1 * pvs_recursive(pos, depth - 1, 1,  -beta, -alpha, uci);
        }
        else {
            if (depth > lmr_min_depth && !pos.is_checked(pos.side_to_move()) && !pos.is_capture(m) && m.type_of() != promotion) {
                score = -1 * pvs_recursive(pos, depth - 1 - get_lmr_reduction(depth, i), 1, -alpha - 1, -alpha, uci);
                if (score > alpha)
                    score = -1 * pvs_recursive(pos, depth - 1, 1, -alpha - 1, -alpha, uci);
            }
            else {
                score = -1 * pvs_recursive(pos, depth - 1, 1, -alpha - 1, -alpha, uci);
            }
            if (score > alpha && beta - alpha > 1)
                score = -1 * pvs_recursive(pos, depth - 1, 1, -beta, -alpha, uci);
        }
        if (nodes_visited % out_of_time_check_freq == 0) {
            check_out_of_time();
            if (uci.stop_search.load()) {
                out_of_time = true;
            }
        }

        if (score > max) {
            max = score;
            best_move = m;
            if (score > alpha) {
                flag = TranspositionFlag::exact;
                alpha = score;
                uci.pv_table.set_pv(m, 0);
            }
        }
        pos.unmake_move(m);
    }
    if (out_of_time)
        return std::make_pair(Move(0), draw_score);

    uci.transposition_table.insert(pos.zobrist, best_move, adjust_score_for_tt(max, 0), depth, flag, age);

    return std::make_pair(best_move, max);
}

Move find_best_move(Position& pos, unsigned long ms_to_search, UCIEngine& uci, int max_depth = 9999) {
    // Prevent age increase from searching same position twice
    // (Won't happen in engine play but maybe analysis idk)
    uci.pv_table.clear_all();
    if (pos.zobrist != prev_zobrist) {
        ++age;
        prev_zobrist = pos.zobrist;
    }
    nodes_visited = 0;
    seldepth = 0;
    out_of_time = false;
    end_time = std::chrono::steady_clock::now() + std::chrono::milliseconds(ms_to_search);

    for (auto& row : uci.killers) {
        row.fill(Move(0));
    }

    int depth = 1;
    Move best_move = Move(0);
    int best_score = negative_inf;
    while (!out_of_time && depth <= max_depth) {
        std::pair<Move, int> curr = pvs(pos, depth, uci);
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