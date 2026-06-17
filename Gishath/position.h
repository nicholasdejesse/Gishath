#pragma once
#include <string>
#include <cmath>
#include <array>
#include "move.h"
#include "types.h"
#include "attacks.h"
#include "movegen.h"

class Move;

struct UnmakeInfo {
    Piece captured_piece = no_piece;
    uint8_t castle_rights = 0;
    uint8_t halfmove_clock = 0;
    int8_t en_passant_file = -1;
};

class Position {
private:
    void move_piece(Square from, Square to);
    void put_piece(Square to, Piece piece);
    void remove_piece(Square from);
    constexpr int unmake_index() const;
    int ply = 0;
    std::array<UnmakeInfo, max_ply> unmake_stack; // Store history, get index through side to move and fullmove clock
    Bitboard zobrist = 0;

public:
    std::array<Piece, square_num> board_mailbox;
    std::array<Bitboard, piece_type_num> type_BB;
    std::array<Bitboard, color_num> color_BB;
    bool white_to_move;
    uint8_t castle_rights;
    uint8_t halfmove_clock;
    uint16_t fullmove_clock;
    int8_t en_passant_file;

    Bitboard pieces();

    std::string to_string();

    void make_move(Move move);
    void unmake_move(Move move);
    bool is_attacked(Square sq, Color us);
    bool is_checked(Color us);
    Bitboard dangerous(Color us);
    bool legal();
    Color to_move() const;
    void load_fen(std::string fen);
    bool has_legal_moves();

    Position();
};