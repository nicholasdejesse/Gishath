#pragma once
#include <string>
#include <iostream>
#include "types.h"

class Move {
public:
    Move(uint16_t data);
    Move(unsigned int from, unsigned int to);
    Move(unsigned int from, unsigned int to, MoveType move_type, PieceType promotion_type = knight);

    Square get_from() const;
    Square get_to() const;
    void set_to(Square to);
    void set_from(Square from);

    MoveType type_of();
    PieceType get_promotion_piece_type();
        
    void operator=(Move a);
    bool operator==(Move a) const;
    bool operator!=(Move a) const;

private:
    uint16_t m_Move;
};

std::ostream& operator<< (std::ostream& stream, const Move& move);