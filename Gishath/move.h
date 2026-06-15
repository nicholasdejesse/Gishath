#pragma once
#include <string>
#include <iostream>
#include "types.h"
#include "position.h"
#include "types.h"

class Position;

class Move {
public:
    Move();
    Move(uint16_t data);
    Move(unsigned int from, unsigned int to);
    Move(unsigned int from, unsigned int to, MoveType move_type, PieceType promotion_type = knight);
    Move(std::string str, Position& pos);

    Square get_from() const;
    Square get_to() const;
    void set_to(Square to);
    void set_from(Square from);

    MoveType type_of();
    PieceType get_promotion_piece_type();
        
    void operator=(Move a);
    bool operator==(Move a) const;
    bool operator!=(Move a) const;
    std::string to_string();

private:
    uint16_t m_Move;
};

