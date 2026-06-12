#include "movegen.h"

void put_pawn_moves(MoveList& ml, Bitboard to, int8_t offset) {
    // Separate pawn push logic since it's faster to move all pawns at once
    while (to) {
        Square sq = pop_lsb(to);
        ml.push(Move(sq - offset, sq));
    }
}

void put_promotion(MoveList& ml, Bitboard to, int8_t offset) {
    while (to) {
        Square sq = pop_lsb(to);
        ml.push(Move(sq - offset, sq, promotion, knight));
        ml.push(Move(sq - offset, sq, promotion, bishop));
        ml.push(Move(sq - offset, sq, promotion, rook));
        ml.push(Move(sq - offset, sq, promotion, queen));
    }
}

void put_moves(MoveList& ml, Square from, Bitboard to) {
    while (to) {
        Square sq = pop_lsb(to);
        ml.push(Move(from, sq));
    }
}

void generate_pawn_moves(Position& pos, MoveList& ml, Color us) {
    Bitboard pawns = pos.type_BB[pawn] & pos.color_BB[us];
    Bitboard empty_squares = ~pos.type_BB[all_pieces];
    Bitboard enemies = pos.color_BB[Color(1 - us)];
    Bitboard promotion_rank = relative_rank(us, rank_7);
    Bitboard promotion_pawns = pawns & promotion_rank;
    Direction up = relative_direction(us, north);
    Direction up_right = relative_direction(us, north_east);
    Direction up_left = relative_direction(us, north_west);

    Bitboard push_1 = shift_BB(pawns & ~promotion_rank, up) & empty_squares;
    Bitboard push_2 = shift_BB(push_1 & ~promotion_rank, up) & empty_squares;
    Bitboard non_promo_captures_left = shift_BB(pawns & ~promotion_rank, up_left) & enemies;
    Bitboard non_promo_captures_right = shift_BB(pawns & ~promotion_rank, up_right) & enemies;

    put_pawn_moves(ml, push_1, up);
    put_pawn_moves(ml, push_2, up + up);
    put_pawn_moves(ml, non_promo_captures_left, up_left);
    put_pawn_moves(ml, non_promo_captures_right, up_right);

    if (promotion_pawns) {
        Bitboard promo_push = shift_BB(pawns, up) & empty_squares;
        Bitboard promo_capture_left = shift_BB(pawns, up_left) & enemies;
        Bitboard promo_capture_right = shift_BB(pawns, up_right) & enemies;

        put_promotion(ml, promo_push, up);
        put_promotion(ml, promo_capture_left, up_left);
        put_promotion(ml, promo_capture_right, up_right);
    }
}

void generate_nonpawn_moves(Position& pos, MoveList& ml, Color us, PieceType pt) {
    Bitboard pt_pieces = pos.type_BB[pt] & pos.color_BB[us];
    Bitboard occ = pos.type_BB[all_pieces];
    Bitboard friendly = pos.color_BB[us];
    
    while (pt_pieces) {
        Square from = pop_lsb(pt_pieces);
        Bitboard to = get_attacks(us, pt, from, occ) & ~friendly;
        put_moves(ml, from, to);
    }
}

// Generates all pseudo-legal moves in a position
void generate(Position& pos, MoveList& ml) {
    Color us = pos.to_move();
    generate_pawn_moves(pos, ml, us);
    for (PieceType pt : {knight, bishop, rook, queen, king}) {
        generate_nonpawn_moves(pos, ml, us, pt);
    }
}