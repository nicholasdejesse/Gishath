#include "move_gen.h"

void MoveList::push(Move m) {
    moves[count++] = m;
}

MoveList::MoveList() : moves{}, count(0) {}


void put_pawn_moves(MoveList& ml, Bitboard to, int8_t offset) {
    // Separate pawn push logic since it's faster to move all pawns at once
    while (to) {
        Square sq = pop_lsb(to);
        ml.push(Move(sq - offset, sq));
    }
}

void put_promotion(MoveList& ml, Bitboard to, int8_t offset, bool queen_only) {
    while (to) {
        Square sq = pop_lsb(to);
        ml.push(Move(sq - offset, sq, promotion, queen));
        if (!queen_only) {
            ml.push(Move(sq - offset, sq, promotion, rook));
            ml.push(Move(sq - offset, sq, promotion, bishop));
            ml.push(Move(sq - offset, sq, promotion, knight));
        }
    }
}

void put_moves(MoveList& ml, Square from, Bitboard to) {
    while (to) {
        Square sq = pop_lsb(to);
        ml.push(Move(from, sq));
    }
}

void generate_pawn_moves(Position& pos, MoveList& ml, Color us, Bitboard mask, bool force_include_promotions) {
    Color them = Color(1 - us);
    Bitboard pawns = pos.type_BB[pawn] & pos.color_BB[us];
    Bitboard empty_squares = ~pos.pieces();
    Bitboard enemies = pos.color_BB[them];
    Bitboard promotion_rank = rank_bb_of(relative_rank(us, rank_7));
    Bitboard promo_pawns = pawns & promotion_rank;
    Bitboard non_promo_pawns = pawns & ~promotion_rank;
    Bitboard second_push_rank = rank_bb_of(relative_rank(us, rank_3));
    Direction up = relative_direction(us, north);
    Direction up_right = relative_direction(us, north_east);
    Direction up_left = relative_direction(us, north_west);

    Bitboard push_1 = shift_BB(non_promo_pawns, up) & empty_squares & mask;
    Bitboard push_2 = shift_BB(push_1 & second_push_rank, up) & empty_squares & mask;
    Bitboard non_promo_captures_left = shift_BB(non_promo_pawns, up_left) & enemies & mask;
    Bitboard non_promo_captures_right = shift_BB(non_promo_pawns, up_right) & enemies & mask;

    put_pawn_moves(ml, push_1, up);
    put_pawn_moves(ml, push_2, up + up);
    put_pawn_moves(ml, non_promo_captures_left, up_left);
    put_pawn_moves(ml, non_promo_captures_right, up_right);

    if (promo_pawns) {
        Bitboard promo_push, promo_capture_left, promo_capture_right;
        if (force_include_promotions) {
            // No mask
            promo_push = shift_BB(promo_pawns, up) & empty_squares;
            promo_capture_left = shift_BB(promo_pawns, up_left) & enemies;
            promo_capture_right = shift_BB(promo_pawns, up_right) & enemies;
            put_promotion(ml, promo_push, up, true);
            put_promotion(ml, promo_capture_left, up_left, true);
            put_promotion(ml, promo_capture_right, up_right, true);
        }
        else {
            promo_push = shift_BB(promo_pawns, up) & empty_squares & mask;
            promo_capture_left = shift_BB(promo_pawns, up_left) & enemies & mask;
            promo_capture_right = shift_BB(promo_pawns, up_right) & enemies & mask;
            put_promotion(ml, promo_push, up, false);
            put_promotion(ml, promo_capture_left, up_left, false);
            put_promotion(ml, promo_capture_right, up_right, false);

        }
    }

    if (pos.en_passant_file != -1) {
        Square target = relative_square(us, make_square(rank_6, pos.en_passant_file)); // Square we want to capture
        Bitboard can_capture = pawn_attacks(them, square_to_bitboard(target)) & pawns & mask;
        while (can_capture) {
            ml.push(Move(pop_lsb(can_capture), target, en_passant));
        }
    }
}

template <GenType type>
void generate_moves(Position& pos, MoveList& ml, Color us, PieceType pt, Bitboard mask) {
    Bitboard pt_pieces = pos.type_BB[pt] & pos.color_BB[us];
    Bitboard occ = pos.pieces();
    Bitboard allowed = ~pos.color_BB[us] & mask;
    
    while (pt_pieces) {
        Square from = pop_lsb(pt_pieces);
        Bitboard to = get_attacks(us, pt, from, occ) & allowed;
        put_moves(ml, from, to);
    }
}

template <GenType type>
void generate_king_moves(Position& pos, MoveList& ml, Color us, Bitboard mask) {
    Bitboard dangerous = pos.dangerous(us);
    Bitboard friendly_king = pos.type_BB[king] & pos.color_BB[us];
    Bitboard allowed = ~pos.color_BB[us] & mask & ~dangerous;

    Square from = get_lsb(friendly_king);
    put_moves(ml, from, get_attacks(us, king, from, pos.pieces()) & allowed);

    if (type == GenType::captures) return;

    // Castling
    Bitboard pieces = pos.pieces();
    bool can_castle = (dangerous & friendly_king) == 0;
    bool can_kingside = us == white ? pos.castle_rights & WK : pos.castle_rights & BK;
    bool can_queenside = us == white ? pos.castle_rights & WQ : pos.castle_rights & BQ;
    if (can_kingside && can_castle) {
        Bitboard castling_sq = relative_bb(us, Bitboard(3) << 5);
        if ((dangerous & castling_sq) == 0 && (castling_sq & pieces) == 0) {
            ml.push(Move(from, relative_square(us, g1), castle));
        }
    }
    if (can_queenside && can_castle) {
        Bitboard castling_sq = relative_bb(us, Bitboard(7) << 1);
        Bitboard intermediate = relative_bb(us, Bitboard(3) << 2);
        if ((dangerous & intermediate) == 0 && (castling_sq & pieces) == 0) {
            ml.push(Move(from, relative_square(us, c1), castle));
        }
    }
}


// Generates all pseudo-legal moves in a position
MoveList generate(Position& pos) {
    MoveList ml;
    //Bitboard check_mask = pos
    Color us = pos.to_move();
    for (PieceType pt : {queen, rook, bishop, knight}) {
        generate_moves<GenType::all>(pos, ml, us, pt, full_bb);
    }
    generate_pawn_moves(pos, ml, us, full_bb, false);
    generate_king_moves<GenType::all>(pos, ml, us, full_bb);
    return ml;
}

template <GenType type>
MoveList generate_by_type(Position& pos) {
    Color us = pos.to_move();
    Bitboard mask = type == GenType::captures ? pos.color_BB[Color(1 - us)]
        : type == GenType::quiet ? ~pos.color_BB[Color(1 - us)]
        : full_bb;

    MoveList ml;
    //Bitboard check_mask = pos
    for (PieceType pt : {queen, rook, bishop, knight}) {
        generate_moves<type>(pos, ml, us, pt, mask);
    }
    bool force_include_promotions = type == GenType::captures;
    generate_pawn_moves(pos, ml, us, mask, force_include_promotions);
    generate_king_moves<type>(pos, ml, us, mask);
    return ml;
}

template MoveList generate_by_type<GenType::captures>(Position&);
template MoveList generate_by_type<GenType::quiet>(Position&);
template MoveList generate_by_type<GenType::all>(Position&);
