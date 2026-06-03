#pragma once
#include <string>
#include "move.h"
#include "types.h"

class Position {
private:
    Piece board_mailbox[SQUARE_NUM];
    Bitboard type_BB[piece_type_num];
    Bitboard color_BB[color_num];
    bool white_to_move;
    uint8_t castle_rights;
    uint8_t halfmove_clock;
    uint16_t fullmove_clock;
    int8_t en_passant_file;

public:
    //void SetFromFen(std::string fen);
    std::string to_string();

    void make_move(Move move);
    void unmake_move(Move move);

    Position();
};