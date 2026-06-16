#include "eval.h"

int evaluate_position(Position& pos) {
    std::array<int, 2> mg{};
    std::array<int, 2> eg{};
    int game_phase = 0;

    /* evaluate each piece */
    for (Color c : {white, black}) {
        for (PieceType pt : {pawn, knight, bishop, rook, queen, king}) {
            Bitboard bb = pos.color_BB[c] & pos.type_BB[pt];
            while (bb) {
                Square sq = pop_lsb(bb);
                mg[c] += mg_table[c * 6 + pt][sq];
                eg[c] += eg_table[c * 6 + pt][sq];
                game_phase += game_phase_inc[pt];
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