#include "move_order.h"
#include "types.h"
#include <cassert>
#include <iostream>
#include "eval.h"

constexpr int hash_score = 90000;
constexpr int capture_base_score = 50000;
constexpr int killer_score_1 = 30000;
constexpr int killer_score_2 = 29000;
constexpr int countermove_score = 28000;
constexpr int good_quiets = 15000;
constexpr int bad_quiets = 7500;

// Note that this includes no_piece_type, but it's faster I think
constexpr std::array<std::array<int, piece_type_num>, piece_type_num> mvv_lva_table{ {
    {0, 0, 0, 0, 0, 0, 0}, // no_piece_type victim
    {0, 16, 15, 14, 13, 12, 11}, // pawn victim
    {0, 26, 25, 24, 23, 22, 21}, // knight victim
    {0, 36, 35, 34, 33, 32, 31}, // bishop victim
    {0, 46, 45, 44, 43, 42, 41}, // rook victim
    {0, 56, 55, 54, 53, 52, 51}, // queen victim
    {0, 0, 0, 0, 0, 0, 0}, // king victim
}};

MoveOrder::MoveOrder(MoveList& ml, Position& pos, int ply, UCIEngine& uci, bool quiescence, Move prev_move) : ml(ml), idx_sorted(0) {
    TranspositionEntry* tt_entry = uci.transposition_table.probe(pos.zobrist);
    for (int i = 0; i < ml.count; ++i) {
        scores[i] = score_move(ml.moves[i], pos, tt_entry, ply, uci, quiescence, prev_move);
    }
}

// Performs selection sort to move next best move to front, then returns it
Move MoveOrder::get_next_move() {
    assert(ml.count > 0);
    if (idx_sorted >= ml.count) {
        assert(losing_captures.count > 0);
        return losing_captures.moves[losing_capture_idx++];
    }
    int max_idx = -1;
    int max_found_score = negative_inf;
    for (int i = idx_sorted; i < ml.count; ++i) {
        if (scores[i] > max_found_score) {
            max_idx = i;
            max_found_score = scores[i];
        }
    }
    Move temp1 = ml.moves[idx_sorted];
    ml.moves[idx_sorted] = ml.moves[max_idx];
    ml.moves[max_idx] = temp1;

    int temp2 = scores[idx_sorted];
    scores[idx_sorted] = scores[max_idx];
    scores[max_idx] = temp2;

    ++idx_sorted;
    return ml.moves[idx_sorted - 1];
}

int MoveOrder::score_move(Move m, Position& pos, TranspositionEntry* tt_entry, int ply, UCIEngine& uci, bool quiescence, Move prev_move) {
    Square from = m.get_from();
    Square to = m.get_to();
    PieceType pt = piecetype_of(pos.board_mailbox[from]);
    PieceType capturing = piecetype_of(pos.board_mailbox[to]);
    Color us = pos.side_to_move();
    if (m.type_of() == en_passant)
        capturing = pawn;

    if (tt_entry) {
        if (m == tt_entry->best_move && pos.pseudo_legal(tt_entry->best_move)) {
            return hash_score;
        }
    }

    //if (capturing != no_piece_type) {
    //    return winning_captures_base_score + mvv_lva_table[capturing][pt];
    //}

    if (capturing != no_piece_type) {
        return capture_base_score + mvv_lva_table[capturing][pt];
    }

    // BLIND sorting (better or lower if not defended)
    // Prioritize captures that gain material or are undefended
    //if (capturing != no_piece_type) {
    //    if (mg_value[capturing - pawn] - mg_value[pt - pawn] < -50 && pos.is_attacked(to, us)) {
    //        return losing_captures_base_score + mvv_lva_table[capturing][pt];
    //    }
    //    else {
    //        return winning_captures_base_score + mvv_lva_table[capturing][pt];
    //    }
    //}

    if (m == uci.killers[ply][0])
        return killer_score_1;
    if (m == uci.killers[ply][1])
        return killer_score_2;

    if (uci.countermove_table[prev_move.get_from()][prev_move.get_to()] == m)
        return countermove_score;

    int history = uci.history_table[pos.side_to_move()][m.get_from()][m.get_to()];
    if (history >= 0) {
        return good_quiets + history;
    }
    return bad_quiets + history;
}

bool MoveOrder::is_losing_capture(Move m, Position& pos) {
    return !pos.see_ge(m, 0);
}

void MoveOrder::push_losing_capture(Move m) {
    losing_captures.push(m);
}

bool MoveOrder::should_search_losing_captures() {
    return idx_sorted >= ml.count; // At end of main move list
}
