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
unsigned long long nodes_visited = 0;
unsigned long long max_nodes_this_search = std::numeric_limits<unsigned long long>::max();
unsigned long long hard_max_nodes_this_search = std::numeric_limits<unsigned long long>::max();
int seldepth = 0;
bool out_of_time = false;
std::chrono::time_point<std::chrono::steady_clock> hard_end_time;
std::chrono::time_point<std::chrono::steady_clock> soft_end_time;

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
    return std::min(depth - 2, static_cast<int>(std::round(0.5 + std::log(static_cast<unsigned int>(depth)) * std::log(static_cast<unsigned int>(move_number)) / 2)));
}

int get_lmp_index(int depth) {
    return 6 + depth * depth;
}

int16_t get_ply_adjusted_mate_score(int score, int ply) {
    if (score >= checkmate_score - max_ply) return score - ply;
    if (score <= -checkmate_score + max_ply) return score + ply;
    return score;
}

void check_out_of_time(UCIEngine& uci) {
    auto time_now = std::chrono::steady_clock::now();
    if (time_now >= hard_end_time) {
        out_of_time = true;
    }
    if (nodes_visited >= hard_max_nodes_this_search) {
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

int quiescence(Position& pos, int depth, int ply, int alpha, int beta, UCIEngine& uci, Move prev_move) {
    if (nodes_visited % out_of_time_check_freq == 0) {
        check_out_of_time(uci);
    }
    if (pos.threefold_or_50_move() || pos.insufficient_material())
        return draw_score;
    if (out_of_time)
        return draw_score; // Doesn't matter what we return here
    ++nodes_visited;

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

    int max = negative_inf;
    int standing_pat = negative_inf;

    bool is_check = pos.is_checked(pos.side_to_move());

    if (!is_check) {
        max = evaluate_position(pos);
        standing_pat = max;
        if (max >= beta)
            return max;
        if (max > alpha)
            alpha = max;
    }

    Color us = pos.side_to_move();

    MoveList ml = is_check ? generate(pos) : generate_by_type<GenType::captures>(pos); // This needs to switch to only generate evasions
    MoveOrder order = MoveOrder(ml, pos, ply, uci, true, prev_move);

    bool found_legal = false;
    Move best_move = Move();
    TranspositionFlag flag = TranspositionFlag::upper_bound;
    for (int i = 0; i < ml.count; ++i) {
        Move m = order.get_next_move();
        PieceType captured_type = piecetype_of(pos.board_mailbox[m.get_to()]);

        pos.make_move(m);
        if (!pos.legal()) {
            pos.unmake_move(m);
            continue;
        }
        found_legal = true;
        // Delta pruning
        if (!is_check && captured_type != no_piece_type && !pos.only_pawns_left(us) && !pos.only_pawns_left(Color(1-us))) {
            int delta_threshold = delta_pruning;
            if (m.type_of() == promotion) {
                delta_threshold += mg_value[queen - pawn];
            }
            if (standing_pat + delta_threshold + mg_value[captured_type - pawn] < alpha) {
                pos.unmake_move(m);
                continue;
            }
        }
        //if (!is_check && !pos.see_ge(m, -150)) {
        //    pos.unmake_move(m);
        //    continue;
        //}
        int score = -1 * quiescence(pos, depth - 1, ply + 1, -beta, -alpha, uci, m);
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
    if (!found_legal && pos.is_checked(us)) {
        return -(checkmate_score - ply);
    }

    uci.transposition_table.insert(pos.zobrist, best_move, adjust_score_for_tt(max, ply), depth, flag, age);
    if (depth < seldepth) seldepth = depth;
    return max;
}

int pvs_recursive(Position& pos, int depth, int ply, int alpha, int beta, UCIEngine& uci, Move prev_move) {
    uci.pv_table.pv_length[ply] = 0;

    // Call every so often since it is expensive
    if (nodes_visited % out_of_time_check_freq == 0) {
        check_out_of_time(uci);
        if (uci.stop_search.load()) {
            out_of_time = true;
        }
    }
    if (out_of_time)
        return draw_score; // Doesn't matter what we return here

    if (pos.threefold_or_50_move() || pos.insufficient_material())
        return draw_score;
    ++nodes_visited;

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

    if (depth <= 0) return quiescence(pos, depth, ply, alpha, beta, uci, prev_move); // Don't flip alpha and beta because we aren't going down search tree this call

    Color us = pos.side_to_move();
    bool is_checked = pos.is_checked(us);
    if (is_checked) depth += 1; // Check extension
    // Null move pruning
    // Don't allow when in check or side to move is only king and pawns (zugzwang)
    if (depth >= null_move_reduction && !pos.is_checked(us) && (pos.color_BB[us] & ~pos.type_BB[pawn] & ~pos.type_BB[king])) {
        pos.make_nullmove();
        int null_score = -1 * pvs_recursive(pos, depth - 1 - null_move_reduction, ply + 1, -beta, -beta + 1, uci, prev_move);
        pos.unmake_nullmove();
        if (out_of_time) {
            return draw_score;
        }

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
    MoveOrder order = MoveOrder(ml, pos, ply, uci, false, prev_move);

    bool found_legal = false;
    bool found_nonfutile = false;

    int max = negative_inf;
    Move best_move = Move();
    TranspositionFlag flag = TranspositionFlag::upper_bound;

    std::array<Move, move_list_size> quiets;
    int quiet_count = 0;
    int legal_moves_searched = 0;

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
        if (legal_moves_searched == 0) {
            score = -1 * pvs_recursive(pos, iter_depth - 1, ply + 1, -beta, -alpha, uci, m);
        }
        else {
            if (!is_checked && !pos.is_checked(pos.side_to_move()) && !is_capture && m.type_of() != promotion) {
                if (iter_depth <= lmp_max_depth && legal_moves_searched >= get_lmp_index(iter_depth) && beta - alpha <= 1) {
                    // Late move pruning
                    pos.unmake_move(m);
                    continue;
                }
                if (iter_depth > lmr_min_depth) {
                    int lmr_reduction = get_lmr_reduction(iter_depth, i);
                    score = -1 * pvs_recursive(pos, iter_depth - 1 - lmr_reduction, ply + 1, -alpha - 1, -alpha, uci, m);
                    if (score > alpha) {
                        score = -1 * pvs_recursive(pos, iter_depth - 1, ply + 1, -alpha - 1, -alpha, uci, m);
                    }
                }
                else {
                    score = -1 * pvs_recursive(pos, iter_depth - 1, ply + 1, -alpha - 1, -alpha, uci, m);
                }
            }
            else {
                score = -1 * pvs_recursive(pos, iter_depth - 1, ply + 1, -alpha - 1, -alpha, uci, m);
            }
            if (score > alpha && beta - alpha > 1)
                score = -1 * pvs_recursive(pos, iter_depth - 1, ply + 1, -beta, -alpha, uci, m);
        }
        if (out_of_time) {
            pos.unmake_move(m);
            return draw_score;
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
            if (!is_capture) {
                Square from = m.get_from();
                Square to = m.get_to();
                uci.countermove_table [prev_move.get_from()][prev_move.get_to()] = m;
                int update_val = iter_depth * iter_depth;
                update_history(uci.history_table, from, to, us, update_val);
                for (int j = 0; j < quiet_count; ++j) {
                    // Apply malus to other quiets that did not cause cutoff
                    update_history(uci.history_table, quiets[j].get_from(), quiets[j].get_to(), us, -update_val);
                }
            }
            return max;
        }
        if (!is_capture) {
            quiets[quiet_count++] = m;
        }
        ++legal_moves_searched;
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

std::pair<Move, int> pvs(Position& pos, int depth, UCIEngine& uci, int alpha, int beta) {
    const int original_alpha = alpha;
    const int original_beta = beta;
    int max = negative_inf;
    Color us = pos.side_to_move();
    bool is_checked = pos.is_checked(us);

    MoveList ml = generate(pos);
    MoveOrder order = MoveOrder(ml, pos, 0, uci, false, Move(0));
    uci.pv_table.pv_length[0] = 0;

    assert(ml.count > 0);

    bool found_legal = false;
    int legal_moves_searched = 0;
    Move best_move = Move(0);
    
    for (int i = 0; i < ml.count; ++i) {
        if (out_of_time)
            return std::make_pair(best_move, draw_score);
        Move m = order.get_next_move();
        bool is_capture = pos.is_capture(m);
        //Move m = ml.moves[i];
        pos.make_move(m);
        if (!pos.legal()) {
            pos.unmake_move(m);
            continue;
        }
        found_legal = true;
        int score;
        if (legal_moves_searched == 0) {
            score = -1 * pvs_recursive(pos, depth - 1, 1,  -beta, -alpha, uci, Move(0));
        }
        else {
            score = -1 * pvs_recursive(pos, depth - 1, 1, -alpha - 1, -alpha, uci, Move(0));
            if (score > alpha && beta - alpha > 1)
                score = -1 * pvs_recursive(pos, depth - 1, 1, -beta, -alpha, uci, Move(0));
        }
        if (nodes_visited % out_of_time_check_freq == 0) {
            check_out_of_time(uci);
            if (uci.stop_search.load()) {
                out_of_time = true;
            }
        }
        if (out_of_time) {
            pos.unmake_move(m);
            return std::make_pair(best_move, draw_score);
        }

        if (score > max) {
            max = score;
            best_move = m;
            if (score > alpha) {
                alpha = score;
                uci.pv_table.set_pv(m, 0);
            }
        }
        pos.unmake_move(m);
        ++legal_moves_searched;
    }
    if (!found_legal) {
        if (is_checked) {
            return std::make_pair(best_move, -checkmate_score);
        }
        else {
            return std::make_pair(best_move, draw_score);
        }
    }
    if (out_of_time)
        return std::make_pair(best_move, draw_score);

    TranspositionFlag flag;
    if (max <= original_alpha)
        flag = TranspositionFlag::upper_bound;
    else if (max >= original_beta)
        flag = TranspositionFlag::lower_bound;
    else
        flag = TranspositionFlag::exact;

    uci.transposition_table.insert(pos.zobrist, best_move, adjust_score_for_tt(max, 0), depth, flag, age);

    return std::make_pair(best_move, max);
}

std::pair<Move, int16_t> find_best_move_and_score(Position& pos, unsigned long ms_to_search, UCIEngine& uci, int max_depth, unsigned long long max_nodes, bool write_info) {
    // Prevent age increase from searching same position twice
    // (Won't happen in engine play but maybe analysis idk)
    uci.pv_table.clear_all();
    if (pos.zobrist != prev_zobrist) {
        ++age;
        prev_zobrist = pos.zobrist;
    }
    nodes_visited = 0;
	max_nodes_this_search = max_nodes;
    if (uci.use_soft_nodes
        && max_nodes <= std::numeric_limits<unsigned long long>::max() / soft_node_hard_limit_multiplier) {
        hard_max_nodes_this_search = max_nodes * soft_node_hard_limit_multiplier;
    }
    else {
        hard_max_nodes_this_search = max_nodes;
    }
    seldepth = 0;
    out_of_time = false;

    std::chrono::time_point<std::chrono::steady_clock> start_time = std::chrono::steady_clock::now();
    hard_end_time = start_time + std::chrono::milliseconds(ms_to_search);
    soft_end_time = start_time + std::chrono::milliseconds(ms_to_search * 2/3);

    for (auto& row : uci.killers) {
        row.fill(Move(0));
    }

    int depth = 1;
    Move prev_move = Move(0);
    Move best_move = Move(0);
    int prev_score = negative_inf;
    int best_score = negative_inf;
    while (!out_of_time && depth <= max_depth) {
        std::pair<Move, int> curr;
        if (depth >= aspiration_min_depth) {
            int alpha_increase = aspiration_window;
            int beta_increase = aspiration_window;
            bool must_research = true;
            for (int i = 0; i < aspiration_max_tries; ++i) {
                int alpha = best_score - alpha_increase;
                int beta = best_score + beta_increase;
                curr = pvs(pos, depth, uci, alpha, beta);
                if (curr.second >= beta) {
                    beta_increase *= 2;
                }
                else if (curr.second <= alpha) {
                    alpha_increase *= 2;
                }
                else {
                    must_research = false;
                    break;
                }
            }
            if (must_research && !out_of_time)
                curr = pvs(pos, depth, uci, negative_inf, positive_inf); // Full research
        }
        else {
            curr = pvs(pos, depth, uci, negative_inf, positive_inf);
        }

        if (!out_of_time) {
            prev_move = best_move;
            prev_score = best_score;
            best_move = curr.first;
            best_score = curr.second;
            int elapsed_ms = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time).count());
            if (write_info)
                uci.write_search_info(depth, std::max(depth, depth - seldepth), best_score, nodes_visited, elapsed_ms);
            ++depth;
        }
	else if (best_move == Move(0)) {
            // Fallback for if we run out of time on the first iteration
			best_move = curr.first;
			best_score = evaluate_position(pos);
		}
		if (nodes_visited >= max_nodes_this_search) {
			break;
		}
        if (std::chrono::steady_clock::now() >= soft_end_time
            && !out_of_time
            && std::abs(best_score - prev_score) < 20
            && prev_move == best_move) {
            break; // Soft end time if search is stable
        }
    }

    return std::make_pair(best_move, best_score);
}

Move find_best_move(Position& pos, unsigned long ms_to_search, UCIEngine& uci, int max_depth, unsigned long long max_nodes, bool write_info) {
    return find_best_move_and_score(pos, ms_to_search, uci, max_depth, max_nodes, write_info).first;
}
