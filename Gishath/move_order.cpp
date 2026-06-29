#include "move_order.h"
#include "types.h"
#include <cassert>
#include <iostream>

constexpr int hash_score = 90000;
constexpr int captures_base_score = 50000;
constexpr int killer_score_1 = 30000;
constexpr int killer_score_2 = 29000;
constexpr int quiet_base_score = 0;

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

MoveOrder::MoveOrder(MoveList& ml, Position& pos, TranspositionTable& transposition_table, std::array<std::array<Move, killers_per_ply>, max_ply>& killers, int ply) : ml(ml), pos(pos), idx_sorted(0), scores{} {
    for (int i = 0; i < ml.count; ++i) {
        scores[i] = score_move(ml.moves[i], pos, transposition_table, killers, ply);
    }
}

// Performs selection sort to move next best move to front, then returns it
Move MoveOrder::get_next_move() {
    assert(ml.count > 0);
    assert(idx_sorted < ml.count);
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

int MoveOrder::score_move(Move m, Position& pos, TranspositionTable& transposition_table, std::array<std::array<Move, killers_per_ply>, max_ply>& killers, int ply) {
    Square from = m.get_from();
    Square to = m.get_to();
    PieceType pt = piecetype_of(pos.board_mailbox[from]);
    PieceType capturing = piecetype_of(pos.board_mailbox[to]);

    TranspositionEntry* tt_entry = transposition_table.probe(pos.zobrist);
    if (tt_entry) {
        if (m == tt_entry->best_move && pos.pseudo_legal(tt_entry->best_move)) {
            return hash_score;
        }
    }

    if (capturing != no_piece_type) {
        return captures_base_score + mvv_lva_table[capturing][pt];
    }

    if (m == killers[ply][0]) {
        return killer_score_1;
    }
    if (m == killers[ply][1]) {
        return killer_score_2;
    }

    return quiet_base_score;
}