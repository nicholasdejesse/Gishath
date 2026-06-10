#pragma once
#include <string>
#include <cmath>
#include "move.h"
#include "types.h"

class Position {
private:
    Piece board_mailbox[square_num];
    Bitboard type_BB[piece_type_num];
    Bitboard color_BB[color_num];
    bool white_to_move;
    uint8_t castle_rights;
    uint8_t halfmove_clock;
    uint16_t fullmove_clock;
    int8_t en_passant_file;

    void move_piece(Square from, Square to);
    void put_piece(Square to, Piece piece);
    void remove_piece(Square from);

public:
    //void SetFromFen(std::string fen);
    std::string to_string();

    void make_move(Move move);
    void unmake_move(Move move);
    void load_fen(std::string fen);

    Position();
};