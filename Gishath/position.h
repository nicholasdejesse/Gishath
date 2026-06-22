#pragma once
#include <string>
#include <cmath>
#include <array>
#include "move.h"
#include "types.h"
#include "attacks.h"
#include "move_gen.h"
#include "game_history.h"
#include "consts.h"

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
    std::array<UnmakeInfo, max_ply> unmake_stack;

public:
    std::array<Piece, square_num> board_mailbox;
    std::array<Bitboard, piece_type_num> type_BB;
    std::array<Bitboard, color_num> color_BB;
    bool white_to_move;
    uint8_t castle_rights;
    uint8_t halfmove_clock;
    uint16_t fullmove_clock;
    int8_t en_passant_file;
    GameHistory history;
    Bitboard zobrist = 0;

    Bitboard pieces();

    std::string to_string();

    void make_move(Move move, bool fen_setup = false);
    void unmake_move(Move move);
    void make_nullmove();
    void unmake_nullmove();
    bool is_attacked(Square sq, Color us);
    bool is_checked(Color us);
    Color side_to_move();
    Bitboard dangerous(Color us);
    bool legal();
    Color to_move() const;
    void load_fen(std::string fen);
    bool has_legal_moves();
    bool threefold_or_50_move();

    Position();
};