#include "position.h"
#include <iostream>
#include <sstream>
#include <cctype>

Position::Position() : board_mailbox{}, type_BB{}, color_BB{}, white_to_move(true), castle_rights(WK | WQ | BK | BQ), halfmove_clock(0), fullmove_clock(1), en_passant_file(-1), unmake_stack{} {}

void Position::load_fen(std::string fen) {
    for (int i = 0; i < square_num; ++i) {
        board_mailbox[i] = no_piece;
    }
    for (int i = 0; i < piece_type_num; ++i) {
        type_BB[i] = 0;
    }
    for (int i = 0; i < color_num; ++i) {
        color_BB[i] = 0;
    }

    std::istringstream ss(fen);
    std::string pieces, side_to_move, castling, en_passant;
    int halfmoves, fullmoves;

    ss >> pieces >> side_to_move >> castling >> en_passant >> halfmoves >> fullmoves;

    // 1. Piece position
    int rank_idx = 7;
    int file_idx = 0;
    for (char c : pieces) {
        if (c == '/') {
            --rank_idx;
            file_idx = 0;
            continue;
        }
        else if (std::isdigit(c)) {
            int empty_squares = c - '0';
            file_idx += empty_squares;
        }
        else {
            Piece piece;
            switch (c) {
            case 'P': piece = white_pawn; break;
            case 'N': piece = white_knight; break;
            case 'B': piece = white_bishop; break;
            case 'R': piece = white_rook; break;
            case 'Q': piece = white_queen; break;
            case 'K': piece = white_king; break;
            case 'p': piece = black_pawn; break;
            case 'n': piece = black_knight; break;
            case 'b': piece = black_bishop; break;
            case 'r': piece = black_rook; break;
            case 'q': piece = black_queen; break;
            case 'k': piece = black_king; break;
            default: throw std::invalid_argument("Invalid FEN string");
            }
            put_piece(Square(rank_idx * 8 + file_idx), piece);
            ++file_idx;
        }
    }

    // 2. Side to move
    if (side_to_move == "w") {
        white_to_move = true;
    }
    else if (side_to_move == "b") {
        white_to_move = false;
    }
    else {
        throw std::invalid_argument("Invalid FEN string");
    }

    // 3. Castling rights
    castle_rights = 0;
    for (char c : castling) {
        switch (c) {
        case 'K': castle_rights |= WK; break;
        case 'Q': castle_rights |= WQ; break;
        case 'k': castle_rights |= BK; break;
        case 'q': castle_rights |= BQ; break;
        case '-': break; // No castling rights
        default: throw std::invalid_argument("Invalid FEN string");
        }
    }
    
    // 4. En passant
    en_passant_file = -1;
    if (en_passant != "-") {
        char file_char = en_passant[0];
        char rank_char = en_passant[1];
        if (file_char < 'a' || file_char > 'h' || rank_char < '1' || rank_char > '8') {
            throw std::invalid_argument("Invalid FEN string");
        }
        en_passant_file = file_char - 'a';
    }
    
    // 5. Halfmove clock
    halfmove_clock = halfmoves;
    
    // 6. Fullmove clock
    fullmove_clock = fullmoves;
}

std::string Position::to_string() {
    std::string piece_to_char = ".PNBRQK  pnbrqk";
    std::string file_markers = "   a b c d e f g h\n";

    std::string str = "";

    if (white_to_move) {
        str += "White ";
    }
    else {
        str += "Black ";
    }
    str += "to move.\n";

    std::string board = "";
    for (int r = 0; r < 8; ++r) {
        std::string row_str = "";
        row_str += std::to_string(r + 1) + "  ";
        for (int c = 0; c < 8; ++c) {
            int square = r * 8 + c;
            row_str += piece_to_char[(static_cast<int>(board_mailbox[square]))];
            row_str += ' ';

        }
        board = "\n" + row_str + board;
    }

    board += "\n\n";

    str = str + board + file_markers;

    str += "En passant file: " + std::to_string(en_passant_file) + "\n";
    str += "Castle rights: " + std::to_string(castle_rights) + "\n";

    return str;
}

constexpr int Position::unmake_index() const {
    return ply;
}

void Position::make_move(Move m) {
    Square from = m.get_from();
    Square to = m.get_to();
    Piece piece = board_mailbox[from];
    PieceType type = piecetype_of(piece);
    Color us = color_of(piece);
    Color them = Color(1 - us);
    MoveType move_type = m.type_of();
    Piece captured_piece = move_type == en_passant ? make_piece(them, pawn) : board_mailbox[to];

    UnmakeInfo unmake = UnmakeInfo{ captured_piece, castle_rights, halfmove_clock, en_passant_file };
    unmake_stack[unmake_index()] = unmake;
    ++ply;

    ++halfmove_clock;
    en_passant_file = -1;
    if (!white_to_move) {
        ++fullmove_clock;
    }
    remove_piece(from);
    if (captured_piece != no_piece) {
        halfmove_clock = 0;
        if (move_type == en_passant) {
            Square pawn_pos = to + relative_direction(us, south);
            remove_piece(pawn_pos);
        }
        else {
            PieceType captured_type = piecetype_of(captured_piece);
            remove_piece(to);
            if (captured_type == rook) {
                if (to == relative_square(them, h1)) {
                    castle_rights &= ~(them == white ? WK : BK);
                }
                else if (to == relative_square(them, a1)) {
                    castle_rights &= ~(them == white ? WQ : BQ);
                }
            }
        }
    }

    if (type == king) {
        castle_rights &= ~(us == white ? (WK | WQ) : (BK | BQ));
    }
    else if (type == pawn) {
        halfmove_clock = 0;
        if (std::abs(static_cast<int>(rank_of(to)) - static_cast<int>(rank_of(from))) > 1) {
            en_passant_file = file_of(to);
        }
    }
    else if (type == rook) {
        if (from == relative_square(us, h1)) {
            castle_rights &= ~(us == white ? WK : BK);
        }
        else if (from == relative_square(us, a1)) {
            castle_rights &= ~(us == white ? WQ : BQ);
        }
    }

    if (move_type == promotion) {
        PieceType promo_type = m.get_promotion_piece_type();
        put_piece(to, make_piece(us, promo_type));
    }
    else {
        put_piece(to, piece);
        if (move_type == castle) {
            bool kingside = (relative_square(us, to) == g1);
            Square from_rook = kingside ? relative_square(us, h1) : relative_square(us, a1);
            Square to_rook = kingside ? relative_square(us, f1) : relative_square(us, d1);
            move_piece(from_rook, to_rook);
        }
    }

    white_to_move = !white_to_move;
}

void Position::unmake_move(Move m) {
    --ply;
    Square from = m.get_from();
    Square to = m.get_to();
    Piece piece = board_mailbox[to];
    PieceType type = piecetype_of(piece);
    Color us = color_of(piece);
    Color them = Color(1 - us);
    MoveType move_type = m.type_of();
    Piece piece_to_restore = m.type_of() == promotion ? make_piece(us, pawn) : piece;

    // Need to do this first so we can get correct index
    white_to_move = !white_to_move;
    if (!white_to_move) {
        --fullmove_clock;
    }

    UnmakeInfo unmake_info = unmake_stack[unmake_index()];

    halfmove_clock = unmake_info.halfmove_clock;
    en_passant_file = unmake_info.en_passant_file;
    castle_rights = unmake_info.castle_rights;

    remove_piece(to);
    if (unmake_info.captured_piece != no_piece) {
        if (move_type == en_passant) {
            Square pawn_pos = to + relative_direction(us, south);
            put_piece(pawn_pos, unmake_info.captured_piece);
        }
        else {
            put_piece(to, unmake_info.captured_piece);
        }
    }

    put_piece(from, piece_to_restore);
    if (move_type == castle) {
        bool kingside = (relative_square(us, to) == g1);
        Square from_rook = kingside ? relative_square(us, h1) : relative_square(us, a1);
        Square to_rook = kingside ? relative_square(us, f1) : relative_square(us, d1);
        move_piece(to_rook, from_rook);
    }
}

void Position::move_piece(Square from, Square to) {
    Piece piece = board_mailbox[from];
    PieceType type = piecetype_of(piece);
    Color color = color_of(piece);
    Bitboard from_to = square_to_bitboard(from) | square_to_bitboard(to);

    type_BB[all_pieces] ^= from_to;
    type_BB[type] ^= from_to;
    color_BB[color] ^= from_to;
    board_mailbox[from] = no_piece;
    board_mailbox[to] = piece;
}
void Position::put_piece(Square to, Piece piece) {
    assert(board_mailbox[to] == no_piece);
    PieceType type = piecetype_of(piece);
    Color color = color_of(piece);
    Bitboard to_mask = square_to_bitboard(to);

    type_BB[all_pieces] ^= to_mask;
    type_BB[type] ^= to_mask;
    color_BB[color] ^= to_mask;
    board_mailbox[to] = piece;
}
void Position::remove_piece(Square from) {
    assert(board_mailbox[from] != no_piece);
    Piece piece = board_mailbox[from];
    PieceType type = piecetype_of(piece);
    Color color = color_of(piece);
    Bitboard from_mask = square_to_bitboard(from);

    type_BB[all_pieces] ^= from_mask;
    type_BB[type] ^= from_mask;
    color_BB[color] ^= from_mask;
    board_mailbox[from] = no_piece;
}
Color Position::to_move() const {
    return white_to_move ? white : black;
}
// Returns true if square is attacked by other side (not us)
bool Position::is_attacked(Square sq, Color us) {
    Bitboard friendly_pieces = color_BB[us];
    Bitboard enemy_pieces = color_BB[Color(1 - us)];
    Bitboard occ = pieces();

    return (get_attacks(us, pawn, sq, occ)   & type_BB[pawn]   & enemy_pieces)
        || (get_attacks(us, knight, sq, occ) & type_BB[knight] & enemy_pieces)
        || (get_attacks(us, bishop, sq, occ) & type_BB[bishop] & enemy_pieces)
        || (get_attacks(us, rook, sq, occ)   & type_BB[rook]   & enemy_pieces)
        || (get_attacks(us, queen, sq, occ)  & type_BB[queen]  & enemy_pieces)
        || (get_attacks(us, king, sq, occ)   & type_BB[king]   & enemy_pieces);
}
bool Position::is_checked(Color us) {
    // Need to change to be faster later
    Square friendly_king = get_lsb(type_BB[king] & color_BB[us]);
    return is_attacked(friendly_king, us);
}
// Returns true if the position is legal
bool Position::legal() {
    Color just_moved = Color(white_to_move ? black : white);
    return !is_checked(just_moved);
}
// Returns the squares that enemies attacking the king reside on and rays to block them
Bitboard Position::dangerous(Color us) {
    Color them = Color(1 - us);
    Bitboard friendly_king = color_BB[us] & type_BB[king];
    Bitboard occ = pieces() & ~friendly_king; // Remove king so it doesn't block sliding pieces
    Bitboard enemies = color_BB[them];

    return (get_attacks(them, pawn, type_BB[pawn] & enemies, occ))
        | (get_attacks(them, knight, type_BB[knight] & enemies, occ))
        | (get_attacks(them, bishop, type_BB[bishop] & enemies, occ))
        | (get_attacks(them, rook, type_BB[rook] & enemies, occ))
        | (get_attacks(them, queen, type_BB[queen] & enemies, occ))
        | (get_attacks(them, king, type_BB[king] & enemies, occ));
}
Bitboard Position::pieces() {
    return type_BB[all_pieces];
}
