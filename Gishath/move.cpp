#include "move.h"
#include "position.h"

Move::Move() = default;
Move::Move(uint16_t data) : m_Move(data) {}
Move::Move(Square from, Square to) : m_Move(((from & 0x3F) << 6) + (to & 0x3F)) {}
Move::Move(Square from, Square to, MoveType move_type, PieceType promotion_type) : m_Move(move_type + ((promotion_type - knight) << 12) + ((from & 0x3F) << 6) + (to & 0x3F)) {}
Move::Move(std::string str, Position& pos) {
    // Parse from/to squares from algebraic notation (e.g. "g7g8q")
    Square from = Square((str[0] - 'a') + (str[1] - '1') * 8);
    Square to = Square((str[2] - 'a') + (str[3] - '1') * 8);

    MoveType move_type = normal;
    PieceType promotion_type = knight; // default, overwritten below if promotion

    Piece moving_piece = pos.board_mailbox[from];
    PieceType moving_piece_type = piecetype_of(moving_piece);

    // Promotion
    if (str.length() == 5) {
        move_type = promotion;
        switch (str[4]) {
        case 'q': promotion_type = queen;  break;
        case 'r': promotion_type = rook;   break;
        case 'b': promotion_type = bishop; break;
        case 'n': promotion_type = knight; break;
        }
    }
    // Castling
    else if (moving_piece_type == king) {
        if (std::abs(static_cast<int>(file_of(to)) - static_cast<int>(file_of(from))) > 1) {
            move_type = castle;
        }
    }
    // En passant
    else if (moving_piece_type == pawn && file_of(from) != file_of(to) && pos.board_mailbox[to] == no_piece) {
        move_type = en_passant;
    }

    m_Move = move_type + ((promotion_type - knight) << 12) + ((from & 0x3F) << 6) + (to & 0x3F);
}
Square Move::get_from() const {
    return Square((m_Move >> 6) & 0x3F);
}

Square Move::get_to() const {
    return Square(m_Move & 0x3F);
}

void Move::set_to(Square to) { m_Move &= ~0x3f; m_Move |= to & 0x3f; }
void Move::set_from(Square from) { m_Move &= ~0xfc0; m_Move |= (from & 0x3f) << 6; }

MoveType Move::type_of() const {
    return MoveType(m_Move & (3 << 14));
}

PieceType Move::get_promotion_piece_type() const {
    return PieceType(((m_Move >> 12) & 3) + knight); // 0 should be knight, so we add knight
}

void Move::operator=(Move a) {
    m_Move = a.m_Move;
}

bool Move::operator==(Move a) const { return (m_Move & 0xffff) == (a.m_Move & 0xffff); }

bool Move::operator!=(Move a) const { return (m_Move & 0xffff) != (a.m_Move & 0xffff); }

std::string Move::to_string() const {
    std::string files = "abcdefgh";
    std::string ranks = "12345678";
    std::string res{ files[file_of(get_from())], ranks[rank_of(get_from())], files[file_of(get_to())], ranks[rank_of(get_to())] };
    if (type_of() == promotion) {
        switch (get_promotion_piece_type()) {
        case knight:
            res += 'n';
            break;
        case bishop:
            res += 'b';
            break;
        case rook:
            res += 'r';
            break;
        case queen:
            res += 'q';
            break;
        }
    }
    return res;
}

bool Move::is_null() {
    return m_Move == 0;
}