#include <algorithm>
#include "attacks.h"
#include "eval.h"
#include "consts.h"
#include "bitboard.h"
#include <cstdlib>

constexpr Bitboard wk_shield_pawn_mask = (Bitboard(7) << 13);// | (Bitboard(7) << 16);
constexpr Bitboard wq_shield_pawn_mask = (Bitboard(7) << 8);// | (Bitboard(7) << 21);
constexpr Bitboard bk_shield_pawn_mask = relative_bb(black, wk_shield_pawn_mask);
constexpr Bitboard bq_shield_pawn_mask = relative_bb(black, wq_shield_pawn_mask);
constexpr Bitboard wk_castle_squares = Bitboard(3) << 6;
constexpr Bitboard wq_castle_squares = Bitboard(7);
constexpr Bitboard bk_castle_squares = relative_bb(black, wk_castle_squares);
constexpr Bitboard bq_castle_squares = relative_bb(black, wq_castle_squares);

int kbnk_corner_distance(Square bishop_sq, Square king_sq) {
    int kf = file_of(king_sq);
    int kr = rank_of(king_sq);

    bool bishop_on_dark = get_square_color(bishop_sq) == black;

    if (bishop_on_dark) {
        // target corners: a1 or h8
        return std::min(kf + kr, (7 - kf) + (7 - kr));
    }
    else {
        // target corners: h1 or a8
        return std::min(kf + (7 - kr), (7 - kf) + kr);
    }
}

int evaluate_position(Position& pos) {
    // Evaluate from white's perspective, then multiply by color_mult at the end
    // Return draw on first repetition to avoid wasted computation
    if (pos.threefold_or_50_move() || pos.insufficient_material()) return draw_score;

    // Material and piece square
    std::array<int, 2> mg{};
    std::array<int, 2> eg{};
    int game_phase = 0;

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

    int mg_score = mg[white] - mg[black];
    int eg_score = eg[white] - eg[black];
    int mg_phase = game_phase;
    if (mg_phase > 24) mg_phase = 24;
    int eg_phase = 24 - mg_phase;
    int eval = (mg_score * mg_phase + eg_score * eg_phase) / 24;

    // Bishop pair
    //if (popcount(pos.type_BB[bishop] & pos.color_BB[white]) >= 2) eval += eval_bishop_pair;
    //if (popcount(pos.type_BB[bishop] & pos.color_BB[black]) >= 2) eval -= eval_bishop_pair;

    // Doubled pawns
    Bitboard doubled_white = pos.type_BB[pawn] & pos.color_BB[white];
    doubled_white = doubled_white & pawn_push_BB(doubled_white, white);
    eval -= popcount(doubled_white) * eval_doubled_pawn_penalty;
    Bitboard doubled_black = pos.type_BB[pawn] & pos.color_BB[black];
    doubled_black = doubled_black & pawn_push_BB(doubled_black, black);
    eval += popcount(doubled_black) * eval_doubled_pawn_penalty;

    // Missing shield pawns when castled (penalty applied if at least two shield pawns are missing since 1 pawn pushed one square is often fine)
    Bitboard wk = pos.type_BB[king] & pos.color_BB[white];
    Bitboard bk = pos.type_BB[king] & pos.color_BB[black];
    if (wk & wk_castle_squares) eval -= std::max(0, 2 - popcount(pos.type_BB[pawn] & pos.color_BB[white] & wk_shield_pawn_mask)) * eval_castle_missing_shield_pawn_penalty * mg_phase / 24;
    else if (wk & wq_castle_squares) eval -= std::max(0, 2 - popcount(pos.type_BB[pawn] & pos.color_BB[white] & wq_shield_pawn_mask)) * eval_castle_missing_shield_pawn_penalty * mg_phase / 24;
    if (bk & wk_castle_squares) eval += std::max(0, 2 - popcount(pos.type_BB[pawn] & pos.color_BB[black] & bk_shield_pawn_mask)) * eval_castle_missing_shield_pawn_penalty * mg_phase / 24;
    else if (bk & wq_castle_squares) eval += std::max(0, 2 - popcount(pos.type_BB[pawn] & pos.color_BB[black] & bq_shield_pawn_mask)) * eval_castle_missing_shield_pawn_penalty * mg_phase / 24;

    // Rook on open/semi-open file
    Bitboard white_rooks = pos.type_BB[rook] & pos.color_BB[white];
    while (white_rooks) {
        Square sq = pop_lsb(white_rooks);
        Bitboard file_mask = file_bb_of(sq);
        if ((file_mask & pos.type_BB[pawn]) == 0) eval += eval_rook_open_file_bonus; // No pawns
        else if ((file_mask & pos.type_BB[pawn] & pos.color_BB[white]) == 0) eval += eval_rook_semi_open_file_bonus; // No friendly pawns
    }
    Bitboard black_rooks = pos.type_BB[rook] & pos.color_BB[black];
    while (black_rooks) {
        Square sq = pop_lsb(black_rooks);
        Bitboard file_mask = file_bb_of(sq);
        if ((file_mask & pos.type_BB[pawn]) == 0) eval -= eval_rook_open_file_bonus;
        else if ((file_mask & pos.type_BB[pawn] & pos.color_BB[black]) == 0) eval -= eval_rook_semi_open_file_bonus;
    }

    // Passed pawns scaled to endgames (bonus for distance to enemy king) and isolated pawns
    Bitboard white_pawns = pos.type_BB[pawn] & pos.color_BB[white];
    while (white_pawns) {
        Square sq = pop_lsb(white_pawns);
        if ((passed_pawn_mask[white][sq] & pos.type_BB[pawn] & pos.color_BB[black]) == 0) {
            eval += (eval_passed_pawn_bonus + manhattan_distance(sq, get_lsb(bk)) + relative_rank(white, rank_of(sq))) * eg_phase / 24;
        }
        Bitboard file_bb = file_bb_of(file_of(sq));
        file_bb = shift_BB(file_bb, east) | shift_BB(file_bb, west);
        if ((file_bb & pos.type_BB[pawn] & pos.color_BB[white]) == 0) {
            eval -= eval_isolated_pawn_penalty;
        }
    }
    Bitboard black_pawns = pos.type_BB[pawn] & pos.color_BB[black];
    while (black_pawns) {
        Square sq = pop_lsb(black_pawns);
        if ((passed_pawn_mask[black][sq] & pos.type_BB[pawn] & pos.color_BB[white]) == 0) {
            eval -= (eval_passed_pawn_bonus + manhattan_distance(sq, get_lsb(wk)) + relative_rank(black, rank_of(sq))) * eg_phase / 24;
        }
        Bitboard file_bb = file_bb_of(file_of(sq));
        file_bb = shift_BB(file_bb, east) | shift_BB(file_bb, west);
        if ((file_bb & pos.type_BB[pawn] & pos.color_BB[black]) == 0) {
            eval += eval_isolated_pawn_penalty;
        }
    }

    // Minor piece mobility
    //Bitboard wpawnattacks = get_attacks(white, pawn, pos.color_BB[white] & pos.type_BB[pawn], pos.pieces());
    //Bitboard bpawnattacks = get_attacks(black, pawn, pos.color_BB[black] & pos.type_BB[pawn], pos.pieces());
    //Bitboard wknight = pos.color_BB[white] & pos.type_BB[knight];
    //while (wknight) {
    //    Square sq = pop_lsb(wknight);
    //    eval += popcount(get_attacks(white, knight, sq, pos.pieces()) & ~bpawnattacks & ~pos.color_BB[white]) * eval_knight_mobility_bonus;
    //}
    //Bitboard wbishop = pos.color_BB[white] & pos.type_BB[bishop];
    //while (wbishop) {
    //    Square sq = pop_lsb(wbishop);
    //    eval += popcount(get_attacks(white, bishop, sq, pos.pieces()) & ~bpawnattacks & ~pos.color_BB[white]) * eval_bishop_mobility_bonus;
    //}
    //Bitboard bknight = pos.color_BB[black] & pos.type_BB[knight];
    //while (bknight) {
    //    Square sq = pop_lsb(bknight);
    //    eval -= popcount(get_attacks(black, knight, sq, pos.pieces()) & ~wpawnattacks & ~pos.color_BB[black]) * eval_knight_mobility_bonus;
    //}
    //Bitboard bbishop = pos.color_BB[black] & pos.type_BB[bishop];
    //while (bbishop) {
    //    Square sq = pop_lsb(bbishop);
    //    eval -= popcount(get_attacks(black, bishop, sq, pos.pieces()) & ~wpawnattacks & ~pos.color_BB[black]) * eval_bishop_mobility_bonus;
    //}

    Square wk_sq = get_lsb(wk);
    Square bk_sq = get_lsb(bk);

    if (pos.type_BB[pawn] == 0) {
        // Mop up (assuming there is sufficient material at this point)
        if ((pos.color_BB[white] & ~pos.type_BB[king]) == 0) {
            // Lone white king
            int edge_dist;
            if (single_pop(pos.color_BB[black] & pos.type_BB[knight])
                && single_pop(pos.color_BB[black] & pos.type_BB[bishop])
                && (pos.color_BB[black] & ~pos.type_BB[knight] & ~pos.type_BB[bishop] & ~pos.type_BB[king]) == 0) {
                // KBNK endgame, push king to correct corner
                edge_dist = 7 - kbnk_corner_distance(get_lsb(pos.color_BB[black] & pos.type_BB[bishop]), wk_sq);
            }
            else {
                edge_dist = std::max(std::abs((static_cast<int>(file_of(wk_sq)) - 3)), std::abs((static_cast<int>(rank_of(wk_sq)) - 3)));
            }
            eval -= edge_dist * eval_mopup_king_on_edge_bonus;
            eval -= (14 - manhattan_distance(wk_sq, bk_sq)) * eval_mopup_dist_between_king_bonus;
        }
        else if ((pos.color_BB[black] & ~pos.type_BB[king]) == 0) {
            // Lone black king
            int edge_dist;
            if (single_pop(pos.color_BB[white] & pos.type_BB[knight])
                && single_pop(pos.color_BB[white] & pos.type_BB[bishop])
                && (pos.color_BB[white] & ~pos.type_BB[knight] & ~pos.type_BB[bishop] & ~pos.type_BB[king]) == 0) {
                // KBNK endgame, push king to correct corner
                edge_dist = 7 - kbnk_corner_distance(get_lsb(pos.color_BB[white] & pos.type_BB[bishop]), bk_sq);
            }
            else {
                edge_dist = std::max(std::abs((static_cast<int>(file_of(bk_sq)) - 3)), std::abs((static_cast<int>(rank_of(bk_sq)) - 3)));
            }
            eval += edge_dist * eval_mopup_king_on_edge_bonus;
            eval += (14 - manhattan_distance(wk_sq, bk_sq)) * eval_mopup_dist_between_king_bonus;
        }
    }

    // King on open file
    Bitboard file_mask = file_bb_of(wk_sq);
    if ((file_mask & pos.type_BB[pawn] & pos.color_BB[white]) == 0) eval -= 10 * mg_phase / 24; // No pawns
    file_mask = file_bb_of(bk_sq);
    if ((file_mask & pos.type_BB[pawn] & pos.color_BB[black]) == 0) eval += 10 * mg_phase / 24; // No pawns

    int color_mult = pos.white_to_move ? 1 : -1;
    return color_mult * eval;
}