#pragma once
#include <string>
#include <cmath>
#include <array>
#include "move.h"
#include "types.h"
#include "attacks.h"

class Position {
private:
    void move_piece(Square from, Square to);
    void put_piece(Square to, Piece piece);
    void remove_piece(Square from);
    constexpr int unmake_index() const;

public:
    std::array<Piece, square_num> board_mailbox;
    std::array<Bitboard, piece_type_num> type_BB;
    std::array<Bitboard, color_num> color_BB;
    bool white_to_move;
    uint8_t castle_rights;
    uint8_t halfmove_clock;
    uint16_t fullmove_clock;
    int8_t en_passant_file;

    std::string to_string();

    void make_move(Move move);
    void unmake_move(Move move);
    bool is_checked(Color us);
    Color to_move() const;
    void load_fen(std::string fen);

    Position();
};