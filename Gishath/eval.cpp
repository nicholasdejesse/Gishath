#include "eval.h"
#include "consts.h"

int evaluate_position(Position& pos) {
    // Return draw on first repetition to avoid wasted computation
    if (pos.history.repeated(pos.zobrist)) return draw_score;
    if (pos.halfmove_clock >= 100) return draw_score;
    // This is a slow way of detecting this, should update later
    if (!pos.has_legal_moves()) {
        if (pos.is_checked(pos.side_to_move())) {
            return checkmate_score;
        }
        else {
            return draw_score;
        }
    }

    std::array<int, 2> mg{};
    std::array<int, 2> eg{};
    int game_phase = 0;

    /* evaluate each piece */
    for (Color c : {white, black}) {
        for (PieceType pt : {pawn, knight, bishop, rook, queen, king}) {
            Bitboard bb = pos.color_BB[c] & pos.type_BB[pt];
            while (bb) {
                Square sq = pop_lsb(bb);
                mg[c] += mg_table[c * 6 + (pt-pawn)][sq];
                eg[c] += eg_table[c * 6 + (pt-pawn)][sq];
                game_phase += game_phase_inc[(pt-pawn)];
            }
        }
    }

    /* tapered eval */
    int mgScore = mg[white] - mg[black];
    int egScore = eg[white] - eg[black];
    int mgPhase = game_phase;
    if (mgPhase > 24) mgPhase = 24; /* in case of early promotion */
    int egPhase = 24 - mgPhase;
    int color_mult = pos.white_to_move ? 1 : -1;
    return color_mult * (mgScore * mgPhase + egScore * egPhase) / 24;
}