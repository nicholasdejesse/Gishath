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
        ml.push(Move(sq - offset, sq, promotion, queen));
        ml.push(Move(sq - offset, sq, promotion, rook));
        ml.push(Move(sq - offset, sq, promotion, bishop));
        ml.push(Move(sq - offset, sq, promotion, knight));
    }
}

void put_moves(MoveList& ml, Square from, Bitboard to) {
    while (to) {
        Square sq = pop_lsb(to);
        ml.push(Move(from, sq));
    }
}

void generate_pawn_moves(Position& pos, MoveList& ml, Color us) {
    Color them = Color(1 - us);
    Bitboard pawns = pos.type_BB[pawn] & pos.color_BB[us];
    Bitboard empty_squares = ~pos.pieces();
    Bitboard enemies = pos.color_BB[them];
    Bitboard promotion_rank = rank_bb_of(relative_rank(us, rank_7));
    Bitboard second_push_rank = rank_bb_of(relative_rank(us, rank_3));
    Bitboard promotion_pawns = pawns & promotion_rank;
    Direction up = relative_direction(us, north);
    Direction up_right = relative_direction(us, north_east);
    Direction up_left = relative_direction(us, north_west);

    Bitboard push_1 = shift_BB(pawns & ~promotion_rank, up) & empty_squares;
    Bitboard push_2 = shift_BB(push_1 & second_push_rank, up) & empty_squares;
    Bitboard non_promo_captures_left = shift_BB(pawns & ~promotion_rank, up_left) & enemies;
    Bitboard non_promo_captures_right = shift_BB(pawns & ~promotion_rank, up_right) & enemies;

    put_pawn_moves(ml, push_1, up);
    put_pawn_moves(ml, push_2, up + up);
    put_pawn_moves(ml, non_promo_captures_left, up_left);
    put_pawn_moves(ml, non_promo_captures_right, up_right);

    if (promotion_pawns) {
        Bitboard promo_push = shift_BB(promotion_pawns, up) & empty_squares;
        Bitboard promo_capture_left = shift_BB(promotion_pawns, up_left) & enemies;
        Bitboard promo_capture_right = shift_BB(promotion_pawns, up_right) & enemies;

        put_promotion(ml, promo_push, up);
        put_promotion(ml, promo_capture_left, up_left);
        put_promotion(ml, promo_capture_right, up_right);
    }

    if (pos.en_passant_file != -1) {
        Square target = relative_square(us, make_square(rank_6, pos.en_passant_file)); // Square we want to capture
        Bitboard can_capture = pawn_attacks(them, square_to_bitboard(target)) & pawns;
        while (can_capture) {
            ml.push(Move(pop_lsb(can_capture), target, en_passant));
        }
    }
}

void generate_moves(Position& pos, MoveList& ml, Color us, PieceType pt) {
    Bitboard pt_pieces = pos.type_BB[pt] & pos.color_BB[us];
    Bitboard occ = pos.pieces();
    Bitboard friendly = pos.color_BB[us];
    
    while (pt_pieces) {
        Square from = pop_lsb(pt_pieces);
        Bitboard to = get_attacks(us, pt, from, occ) & ~friendly;
        put_moves(ml, from, to);
    }
}

void generate_king_moves(Position& pos, MoveList& ml, Color us) {
    Bitboard dangerous = pos.dangerous(us);
    Bitboard friendly_king = pos.type_BB[king] & pos.color_BB[us];
    Bitboard friendly = pos.color_BB[us];

    Square from = get_lsb(friendly_king);
    put_moves(ml, from, get_attacks(us, king, from, pos.pieces()) & ~friendly & ~dangerous);

    Bitboard pieces = pos.pieces();
    bool can_castle = (dangerous & friendly_king) == 0;
    bool can_kingside = us == white ? pos.castle_rights & WK : pos.castle_rights & BK;
    bool can_queenside = us == white ? pos.castle_rights & WQ : pos.castle_rights & BQ;
    if (can_kingside && can_castle) {
        Square s1 = relative_square(us, f1);
        Square s2 = relative_square(us, g1);
        Square intermediate = relative_square(us, f1);
        if ((dangerous & intermediate) == 0 && pos.board_mailbox[s1] == no_piece && pos.board_mailbox[s2] == no_piece) {
            ml.push(Move(from, relative_square(us, g1), castle));
        }
    }
    if (can_queenside && can_castle) {
        Square s1 = relative_square(us, b1);
        Square s2 = relative_square(us, c1);
        Square s3 = relative_square(us, d1);
        Square intermediate = relative_square(us, d1);
        if ((dangerous & intermediate) == 0 && pos.board_mailbox[s1] == no_piece && pos.board_mailbox[s2] == no_piece && pos.board_mailbox[s3] == no_piece) {
            ml.push(Move(from, relative_square(us, c1), castle));
        }
    }
}


// Generates all pseudo-legal moves in a position
void generate(Position& pos, MoveList& ml) {
    //Bitboard check_mask = pos
    Color us = pos.to_move();
    generate_pawn_moves(pos, ml, us);
    for (PieceType pt : {knight, bishop, rook, queen}) {
        generate_moves(pos, ml, us, pt);
    }
    generate_king_moves(pos, ml, us);
}