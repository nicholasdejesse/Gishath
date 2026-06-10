#pragma once
#include <string>

#include "types.h"

class Move {
public:
    Move(uint16_t data);
    Move(unsigned int from, unsigned int to);
    Move(unsigned int from, unsigned int to, MoveType move_type, PieceType promotion_type = knight);

    Square get_from();
    Square get_to();
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