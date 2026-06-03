#include "position.h"

Position::Position() : board_mailbox{}, type_BB{}, color_BB{}, white_to_move(true), castle_rights(WK | WQ | BK | BQ), halfmove_clock(0), fullmove_clock(0), en_passant_file(-1) {}

std::string Position::to_string() {
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
        for (int c = 0; c < 8; ++c) {
            int square = r * 8 + c;
            row_str += std::to_string(static_cast<int>(board_mailbox[square])) + "\t";
        }
        board = "\n" + row_str + board;
    }

    return str + board;
}

void Position::make_move(Move move) {
    ++halfmove_clock;
    ++fullmove_clock;

    Square from = move.get_from();
    Square to = move.get_to();
    Piece piece = board_mailbox[from];
    PieceType type = type_of(piece);
    Color color = color_of(piece);

    board_mailbox[from] = no_piece;
    board_mailbox[to] = piece;
    Bitboard from_mask = static_cast<Bitboard>(1) << from;
    Bitboard to_mask = static_cast<Bitboard>(1) << to;

    type_BB[type] ^= from_mask;
    color_BB[color] ^= from_mask;
    color_BB[color] ^= to_mask;

    // TODO: to mask, captures, castling, en passant file, 50 move rule

    if (type == king) {
        castle_rights = 0;
    }

    if (move.is_promotion()) {
        PieceType promo_type = move.get_promotion_piece_type();
        type_BB[promo_type] ^= to_mask;
    }
    else {
        type_BB[type] ^= to_mask;
        if (move.is_castle()) {

        }
    }

    white_to_move = !white_to_move;
}