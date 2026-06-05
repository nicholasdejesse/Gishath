#include "position.h"
#include <iostream>

Position::Position() : board_mailbox{}, type_BB{}, color_BB{}, white_to_move(true), castle_rights(WK | WQ | BK | BQ), halfmove_clock(0), fullmove_clock(1), en_passant_file(-1) {}

std::string Position::to_string() {
    std::string piece_to_char = ".PNBRQK  pnbrqk";
    std::string file_markers = "   a b c d e f g h\n";

    std::string str = "";

    if (white_to_move) {
        str += "White ";
    }
    else {
        str += "Black ";
    }
    str += "to move.\n";

    std::string board = "";
    for (int r = 0; r < 8; ++r) {
        std::string row_str = "";
        row_str += std::to_string(r + 1) + "  ";
        for (int c = 0; c < 8; ++c) {
            int square = r * 8 + c;
            row_str += piece_to_char[(static_cast<int>(board_mailbox[square]))];
            row_str += ' ';

        }
        board = "\n" + row_str + board;
    }

    board += "\n\n";

    str = str + board + file_markers;

    return str;
}

void Position::make_move(Move m) {
    Square from = m.get_from();
    Square to = m.get_to();
    Piece piece = board_mailbox[from];
    PieceType type = type_of(piece);
    Color us = color_of(piece);
    Color them = Color(1 - us);
    MoveType move_type = m.type_of();
    Piece captured_piece = move_type == en_passant ? make_piece(them, pawn) : board_mailbox[to];
    Bitboard from_mask = square_to_bitboard(from);
    Bitboard to_mask = square_to_bitboard(to);

    ++halfmove_clock;
    en_passant_file = -1;
    if (!white_to_move) {
        ++fullmove_clock;
    }

    type_BB[type] ^= from_mask;
    color_BB[us] ^= from_mask;
    board_mailbox[from] = no_piece;

    color_BB[us] ^= to_mask;

    if (captured_piece != no_piece) {
        if (move_type == en_passant) {
            Square pawn_pos = to - relative_direction(us, south);
            type_BB[pawn] ^= square_to_bitboard(pawn_pos);
            color_BB[them] ^= square_to_bitboard(pawn_pos);
            board_mailbox[pawn_pos] = no_piece;
            halfmove_clock = 0;
        }
        else {
            PieceType captured_type = type_of(captured_piece);
            type_BB[captured_type] ^= to_mask;
            color_BB[them] ^= to_mask;
            halfmove_clock = 0;
            if (captured_type == rook) {
                if (to == relative_square(them, h1)) {
                    castle_rights &= ~(them == white ? WK : BK);
                }
                else if (to == relative_square(them, a1)) {
                    castle_rights &= ~(them == white ? WQ : BQ);
                }
            }
        }
    }

    if (type == king) {
        castle_rights = 0;
    }
    else if (type == pawn) {
        halfmove_clock = 0;
        if (std::abs(to - from) > north) {
            en_passant_file = file_of(to);
        }
    }
    else if (type == rook) {
        if (from == relative_square(us, h1)) {
            castle_rights &= ~(us == white ? WK : BK);
        }
        else if (from == relative_square(us, a1)) {
            castle_rights &= ~(us == white ? WQ : BQ);
        }
    }

    if (move_type == promotion) {
        PieceType promo_type = m.get_promotion_piece_type();
        type_BB[promo_type] ^= to_mask;
        board_mailbox[to] = Piece(us * 8 + promo_type);
    }
    else {
        type_BB[type] ^= to_mask;
        board_mailbox[to] = piece;
        if (move_type == castle) {
            bool kingside = (relative_square(us, to) == g1);
            Square from_rook = kingside ? relative_square(us, h1) : relative_square(us, a1);
            Square to_rook = kingside ? relative_square(us, f1) : relative_square(us, d1);
            type_BB[rook] ^= square_to_bitboard(from_rook);
            color_BB[us] ^= square_to_bitboard(from_rook);
            type_BB[rook] ^= square_to_bitboard(to_rook);
            color_BB[us] ^= square_to_bitboard(to_rook);
            board_mailbox[from_rook] = no_piece;
            board_mailbox[to_rook] = us == white ? white_rook : black_rook;
        }
    }

    white_to_move = !white_to_move;
}

void Position::unmake_move(Move m) {

}