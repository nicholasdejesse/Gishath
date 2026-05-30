#include "move.h"

Move::Move(unsigned int from, unsigned int to, unsigned int flags) {
    m_Move = ((flags & 0xF) << 12) | ((from & 0x3F) << 6) | (to & 0x3F);
}

unsigned int Move::getFrom() {
    return (m_Move >> 6) & 0x3F;
}

unsigned int Move::getTo() {
    return m_Move & 0x3F;
}

unsigned int Move::getFlags() {
    return (m_Move >> 12) & 0xF;
}

void Move::setTo(unsigned int to) { m_Move &= ~0x3f; m_Move |= to & 0x3f; }
void Move::setFrom(unsigned int from) { m_Move &= ~0xfc0; m_Move |= (from & 0x3f) << 6; }

void Move::operator=(Move a) {
    m_Move = a.m_Move;
}

bool Move::operator==(Move a) const { return (m_Move & 0xffff) == (a.m_Move & 0xffff); }

bool Move::operator!=(Move a) const { return (m_Move & 0xffff) != (a.m_Move & 0xffff); }
