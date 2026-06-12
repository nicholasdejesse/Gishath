#include "move.h"

Move::Move() = default;
Move::Move(uint16_t data) : m_Move(data) {}
Move::Move(unsigned int from, unsigned int to) : m_Move(((from & 0x3F) << 6) + (to & 0x3F)) {}
Move::Move(unsigned int from, unsigned int to, MoveType move_type, PieceType promotion_type) : m_Move(move_type + ((promotion_type - knight) << 12) + ((from & 0x3F) << 6) + (to & 0x3F)) {}

Square Move::get_from() const {
    return Square((m_Move >> 6) & 0x3F);
}

Square Move::get_to() const {
    return Square(m_Move & 0x3F);
}

void Move::set_to(Square to) { m_Move &= ~0x3f; m_Move |= to & 0x3f; }
void Move::set_from(Square from) { m_Move &= ~0xfc0; m_Move |= (from & 0x3f) << 6; }

MoveType Move::type_of() {
    return MoveType(m_Move & (3 << 14));
}

PieceType Move::get_promotion_piece_type() {
    return PieceType(((m_Move >> 12) & 3) + knight); // 0 should be knight, so we add knight
}

void Move::operator=(Move a) {
    m_Move = a.m_Move;
}

bool Move::operator==(Move a) const { return (m_Move & 0xffff) == (a.m_Move & 0xffff); }

bool Move::operator!=(Move a) const { return (m_Move & 0xffff) != (a.m_Move & 0xffff); }

std::ostream& operator<< (std::ostream& stream, const Move& move) {
    std::string files = "abcdefgh";
    std::string ranks = "12345678";
    stream << files[file_of(move.get_from())] << ranks[rank_of(move.get_from())] << files[file_of(move.get_to())] << ranks[rank_of(move.get_to())];

    return stream;
}