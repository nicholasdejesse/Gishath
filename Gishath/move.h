#pragma once
#include <string>
#include "types.h"

class Move {
public:
    Move(unsigned int from, unsigned int to, unsigned int flags);
    Square get_from();
    Square get_to();
    unsigned int get_flags(); // Flags use https://www.chessprogramming.org/Encoding_Moves
    void set_to(Square to);
    void set_from(Square from);

    bool is_castle();
    bool is_promotion();
    bool is_capture();
    PieceType get_promotion_piece_type();
        
    void operator=(Move a);
    bool operator==(Move a) const;
    bool operator!=(Move a) const;

protected:
    unsigned int m_Move;
};