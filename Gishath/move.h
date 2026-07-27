#pragma once
#include <string>
#include <iostream>
#include "types.h"

class Position;

// Based on stockfish move encoding
// Bits 0-5: to square (0..63)
// Bits 6-11: from square (0..63)
// Bits 12-13: promotion piece type (0..3) (knight, bishop, rook, queen)
// Bit 14-15: special move flag (0..3) (quiet, promotion, castle, en passant)

class Move {
public:
    Move();
    Move(uint16_t data);
    Move(Square from, Square to);
    Move(Square from, Square to, MoveType move_type, PieceType promotion_type = knight);
    Move(std::string str, Position& pos);

    Square get_from() const;
    Square get_to() const;
    void set_to(Square to);
    void set_from(Square from);

    MoveType type_of() const;
    PieceType get_promotion_piece_type() const;
    bool is_null();
        
    void operator=(Move a);
    bool operator==(Move a) const;
    bool operator!=(Move a) const;
    std::string to_string() const;

private:
    uint16_t m_Move;
};

