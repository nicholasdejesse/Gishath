#include "move.h"

Move::Move(unsigned int from, unsigned int to, unsigned int flags) {
    m_Move = ((flags & 0xF) << 12) | ((from & 0x3F) << 6) | (to & 0x3F);
}

Square Move::get_from() {
    return Square((m_Move >> 6) & 0x3F);
}

Square Move::get_to() {
    return Square(m_Move & 0x3F);
}

unsigned int Move::get_flags() {
    return (m_Move >> 12) & 0xF;
}

void Move::set_to(Square to) { m_Move &= ~0x3f; m_Move |= to & 0x3f; }
void Move::set_from(Square from) { m_Move &= ~0xfc0; m_Move |= (from & 0x3f) << 6; }

bool Move::is_castle() {
    return (get_flags() >> 1) == 1;
}

bool Move::is_promotion() {
    return (get_flags() >> 3) == 1;
}

bool Move::is_capture() {
    return (get_flags() & 4) > 0;
}

PieceType Move::get_promotion_piece_type() {
    return PieceType((get_flags() & 7) + knight);
}

void Move::operator=(Move a) {
    m_Move = a.m_Move;
}

bool Move::operator==(Move a) const { return (m_Move & 0xffff) == (a.m_Move & 0xffff); }

bool Move::operator!=(Move a) const { return (m_Move & 0xffff) != (a.m_Move & 0xffff); }
