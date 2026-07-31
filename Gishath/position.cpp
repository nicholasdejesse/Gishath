#include "position.h"
#include "zobrist.h"
#include <iostream>
#include <sstream>
#include <cctype>
#include "eval.h"

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
            Square sq = Square(rank_idx * 8 + file_idx);
            put_piece(sq, piece);
            zobrist ^= zobrist_pieces[compact_piece_idx(piece)][sq];
            ++file_idx;
        }
    }

    // 2. Side to move
    if (side_to_move == "w") {
        white_to_move = true;
    }
    else if (side_to_move == "b") {
        white_to_move = false;
        zobrist ^= zobrist_black_to_move;
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

    assert(castle_rights < 16);
    zobrist ^= zobrist_castling[castle_rights & 0xf];
    
    // 4. En passant
    en_passant_file = -1;
    if (en_passant != "-") {
        char file_char = en_passant[0];
        char rank_char = en_passant[1];
        if (file_char < 'a' || file_char > 'h' || rank_char < '1' || rank_char > '8') {
            throw std::invalid_argument("Invalid FEN string");
        }
        en_passant_file = file_char - 'a';
        zobrist ^= zobrist_en_passant_file[en_passant_file];
    }
    
    // 5. Halfmove clock
    halfmove_clock = halfmoves;
    
    // 6. Fullmove clock
    fullmove_clock = fullmoves;

    // Load to game history
    history = GameHistory();
    history.push(zobrist, false);
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

void Position::make_move(Move m, bool fen_setup) {
    Square from = m.get_from();
    Square to = m.get_to();
    Piece piece = board_mailbox[from];
    PieceType type = piecetype_of(piece);
    Color us = color_of(piece);
    Color them = Color(1 - us);
    MoveType move_type = m.type_of();
    Piece captured_piece = move_type == en_passant ? make_piece(them, pawn) : board_mailbox[to];

    if (!fen_setup) {
        UnmakeInfo unmake = UnmakeInfo{ captured_piece, castle_rights, halfmove_clock, en_passant_file };
        unmake_stack[unmake_index()] = unmake;
        ++ply;
    }
    bool is_reversible = true;

    ++halfmove_clock;
    if (en_passant_file != -1) {
        zobrist ^= zobrist_en_passant_file[en_passant_file];
        en_passant_file = -1;
    }
    if (!white_to_move) {
        ++fullmove_clock;
    }
    remove_piece(from);
    if (captured_piece != no_piece) {
        is_reversible = false;
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
                    zobrist ^= zobrist_castling[castle_rights];
                    castle_rights &= ~(them == white ? WK : BK);
                    zobrist ^= zobrist_castling[castle_rights];
                }
                else if (to == relative_square(them, a1)) {
                    zobrist ^= zobrist_castling[castle_rights];
                    castle_rights &= ~(them == white ? WQ : BQ);
                    zobrist ^= zobrist_castling[castle_rights];
                }
            }
        }
    }

    if (type == king) {
        zobrist ^= zobrist_castling[castle_rights];
        castle_rights &= ~(us == white ? (WK | WQ) : (BK | BQ));
        zobrist ^= zobrist_castling[castle_rights];
    }
    else if (type == pawn) {
        is_reversible = false;
        halfmove_clock = 0;
        if (std::abs(static_cast<int>(rank_of(to)) - static_cast<int>(rank_of(from))) > 1) {
            en_passant_file = file_of(to);
            zobrist ^= zobrist_en_passant_file[en_passant_file];
        }
    }
    else if (type == rook) {
        if (from == relative_square(us, h1)) {
            zobrist ^= zobrist_castling[castle_rights];
            castle_rights &= ~(us == white ? WK : BK);
            zobrist ^= zobrist_castling[castle_rights];
        }
        else if (from == relative_square(us, a1)) {
            zobrist ^= zobrist_castling[castle_rights];
            castle_rights &= ~(us == white ? WQ : BQ);
            zobrist ^= zobrist_castling[castle_rights];
        }
    }

    if (move_type == promotion) {
        PieceType promo_type = m.get_promotion_piece_type();
        put_piece(to, make_piece(us, promo_type));
    }
    else {
        put_piece(to, piece);
        if (move_type == castle) {
            is_reversible = false;
            bool kingside = (relative_square(us, to) == g1);
            Square from_rook = kingside ? relative_square(us, h1) : relative_square(us, a1);
            Square to_rook = kingside ? relative_square(us, f1) : relative_square(us, d1);
            move_piece(from_rook, to_rook);
        }
    }

    white_to_move = !white_to_move;
    zobrist ^= zobrist_black_to_move;
    history.push(zobrist, is_reversible);
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
    zobrist ^= zobrist_black_to_move;
    if (!white_to_move) {
        --fullmove_clock;
    }

    UnmakeInfo unmake_info = unmake_stack[unmake_index()];

    if (en_passant_file != -1) {
        zobrist ^= zobrist_en_passant_file[en_passant_file];
    }
    zobrist ^= zobrist_castling[castle_rights];
    halfmove_clock = unmake_info.halfmove_clock;
    en_passant_file = unmake_info.en_passant_file;
    castle_rights = unmake_info.castle_rights;
    zobrist ^= zobrist_castling[castle_rights];
    if (en_passant_file != -1) {
        zobrist ^= zobrist_en_passant_file[en_passant_file];
    }

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
    history.pop();
}

void Position::make_nullmove() {
    UnmakeInfo unmake = UnmakeInfo{ no_piece, castle_rights, halfmove_clock, en_passant_file };
    unmake_stack[unmake_index()] = unmake;
    ++ply;

    ++halfmove_clock;
    if (en_passant_file != -1) {
        zobrist ^= zobrist_en_passant_file[en_passant_file];
        en_passant_file = -1;
    }
    if (!white_to_move) {
        ++fullmove_clock;
    }

    white_to_move = !white_to_move;
    zobrist ^= zobrist_black_to_move;
}

void Position::unmake_nullmove() {
    --ply;

    // Need to do this first so we can get correct index
    white_to_move = !white_to_move;
    zobrist ^= zobrist_black_to_move;
    if (!white_to_move) {
        --fullmove_clock;
    }

    UnmakeInfo unmake_info = unmake_stack[unmake_index()];

    if (en_passant_file != -1) {
        zobrist ^= zobrist_en_passant_file[en_passant_file];
    }
    halfmove_clock = unmake_info.halfmove_clock;
    en_passant_file = unmake_info.en_passant_file;
    if (en_passant_file != -1) {
        zobrist ^= zobrist_en_passant_file[en_passant_file];
    }
}

void Position::move_piece(Square from, Square to) {
    Piece piece = board_mailbox[from];
    PieceType type = piecetype_of(piece);
    Color color = color_of(piece);
    Bitboard from_to = square_to_bitboard(from) | square_to_bitboard(to);

    zobrist ^= zobrist_pieces[compact_piece_idx(piece)][from];
    zobrist ^= zobrist_pieces[compact_piece_idx(piece)][to];
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

    zobrist ^= zobrist_pieces[compact_piece_idx(piece)][to];
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

    zobrist ^= zobrist_pieces[compact_piece_idx(piece)][from];
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
    assert(sq <= h8 && sq >= a1);
    Bitboard enemy_pieces = color_BB[Color(1 - us)];
    Bitboard occ = pieces();

    return (get_attacks(us, pawn, sq, occ)   & type_BB[pawn]   & enemy_pieces)
        || (get_attacks(us, knight, sq, occ) & type_BB[knight] & enemy_pieces)
        || (get_attacks(us, bishop, sq, occ) & type_BB[bishop] & enemy_pieces)
        || (get_attacks(us, rook, sq, occ)   & type_BB[rook]   & enemy_pieces)
        || (get_attacks(us, queen, sq, occ)  & type_BB[queen]  & enemy_pieces)
        || (get_attacks(us, king, sq, occ)   & type_BB[king]   & enemy_pieces);
}

Bitboard Position::attackers(Square sq, Color side, Bitboard occ) {
    assert(sq <= h8 && sq >= a1);
    Bitboard our_pieces = color_BB[side];
    Color other = Color(1 - side);

    return (get_attacks(other, pawn, sq, occ) & type_BB[pawn] & our_pieces)
        | (get_attacks(other, knight, sq, occ) & type_BB[knight] & our_pieces)
        | (get_attacks(other, bishop, sq, occ) & type_BB[bishop] & our_pieces)
        | (get_attacks(other, rook, sq, occ) & type_BB[rook] & our_pieces)
        | (get_attacks(other, queen, sq, occ) & type_BB[queen] & our_pieces)
        | (get_attacks(other, king, sq, occ) & type_BB[king] & our_pieces);
}

bool Position::is_checked(Color us) {
    // Need to change to be faster later
    assert((type_BB[king] & color_BB[us]) > 0);
    Square friendly_king = get_lsb(type_BB[king] & color_BB[us]);
    return is_attacked(friendly_king, us);
}
bool Position::has_legal_moves() {
    MoveList ml = generate(*this);
    if (ml.count == 0) return false;
    for (int i = 0; i < ml.count; ++i) {
        make_move(ml.moves[i]);
        if (legal()) {
            unmake_move(ml.moves[i]);
            return true;
        }
        unmake_move(ml.moves[i]);
    }
    return false;
}
// Returns true if the position is legal
bool Position::legal() {
    Color just_moved = Color(white_to_move ? black : white);
    return !is_checked(just_moved);
}
Color Position::side_to_move() {
    return white_to_move ? white : black;
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
bool Position::threefold_or_50_move() {
    return history.repeated(zobrist) || halfmove_clock >= 100;
}
// Determine if move is pseudo-legal (for transposition table)
bool Position::pseudo_legal(Move m) {
    if (m.is_null()) return true;

    Square from = m.get_from();

    // Piece exists
    if (board_mailbox[from] == no_piece) return false;

    Square to = m.get_to();
    Piece piece = board_mailbox[from];
    PieceType type = piecetype_of(piece);
    Color us = color_of(piece);

    // Not opponent's piece
    if (us != side_to_move()) return false;

    // Not capturing our own piece
    if (square_to_bitboard(to) & color_BB[us]) return false;

    // Clear path for sliding 
    if ((line_between[from][to] & pieces()) != 0) return false;

    // Special cases for castling and en passant
    MoveType move_type = m.type_of();
    if (move_type == castle) {
        bool kingside = (relative_square(us, to) == g1);
        uint8_t us_kingside_rights = us == white ? WK : WQ;
        uint8_t us_queenside_rights = us == white ? WQ : BQ;
        if (kingside) {
            if ((castle_rights & us_kingside_rights) == 0 || is_attacked(relative_square(us, f1), us)) return false;
        }
        else {
            if ((castle_rights & us_queenside_rights) == 0 || is_attacked(relative_square(us, d1), us)) return false;
        }
    }
    else if (move_type == en_passant) {
        if (file_of(to) != en_passant_file) return false;
    }

    return true;
}
bool Position::insufficient_material() {
    if (type_BB[pawn] || type_BB[rook] || type_BB[queen]) return false;
    return popcount(pieces()) < 4;
}
bool Position::is_capture(Move m) {
    if (m.type_of() == en_passant) return true;
    return color_BB[Color(1-side_to_move())] & square_to_bitboard(m.get_to());
}

// Return true if static exchange evaluation is at least threshold
// from the perspective of the side making the first move.
bool Position::see_ge(Move m, int threshold) {
    std::array<int, 32> gains{};
    int gain_idx = 0;

    Color us = side_to_move();
    Color them = Color(1 - us);
    Square to = m.get_to();
    Bitboard occ = pieces();

    PieceType captured_type = piecetype_of(board_mailbox[to]);
    if (m.type_of() == en_passant)
        captured_type = pawn;

    if (captured_type == no_piece_type)
        return false;

    // Seed with the value of the captured piece minus the requested threshold.
    gains[gain_idx++] = mg_value[captured_type - pawn] - threshold;

    PieceType moving_type = piecetype_of(board_mailbox[m.get_from()]);
    if (m.type_of() == promotion) { // or whatever your API uses
        moving_type = m.get_promotion_piece_type();
    }
    int attack_square_val = mg_value[moving_type - pawn];

    occ ^= square_to_bitboard(m.get_from());
    if (m.type_of() == en_passant)
        occ ^= shift_BB(square_to_bitboard(m.get_to()), relative_direction(us, south));
    else
        occ ^= square_to_bitboard(m.get_to());

    Bitboard attacker = attackers(to, us, occ) | attackers(to, them, occ);
    if ((attacker & color_BB[them]) == 0)
        return gains[0] >= 0;

    Bitboard bb;

    while (true) {
        us = Color(1 - us);
        attacker &= occ;

        auto [pinners, blockers] = get_pinners_and_blockers(us, occ);
        if (pinners & occ) {
            attacker &= ~blockers;
        }

        if (!(attacker & color_BB[us]))
            break;

        if ((bb = attacker & color_BB[us] & type_BB[pawn])) {
            Square att_sq = pop_lsb(bb);
            gains[gain_idx] = attack_square_val - gains[gain_idx - 1];
            ++gain_idx;
            attack_square_val = mg_value[pawn - pawn];
            occ ^= square_to_bitboard(att_sq);
            attacker ^= square_to_bitboard(att_sq);
            attacker |= get_attacks(us, bishop, to, occ) & color_BB[us] & (type_BB[bishop] | type_BB[queen]);
        }
        else if ((bb = attacker & color_BB[us] & type_BB[knight])) {
            Square att_sq = pop_lsb(bb);
            gains[gain_idx] = attack_square_val - gains[gain_idx - 1];
            ++gain_idx;
            attack_square_val = mg_value[knight - pawn];
            occ ^= square_to_bitboard(att_sq);
            attacker ^= square_to_bitboard(att_sq);
        }
        else if ((bb = attacker & color_BB[us] & type_BB[bishop])) {
            Square att_sq = pop_lsb(bb);
            gains[gain_idx] = attack_square_val - gains[gain_idx - 1];
            ++gain_idx;
            attack_square_val = mg_value[bishop - pawn];
            occ ^= square_to_bitboard(att_sq);
            attacker ^= square_to_bitboard(att_sq);
            attacker |= get_attacks(us, bishop, to, occ) & color_BB[us] & (type_BB[bishop] | type_BB[queen]);
        }
        else if ((bb = attacker & color_BB[us] & type_BB[rook])) {
            Square att_sq = pop_lsb(bb);
            gains[gain_idx] = attack_square_val - gains[gain_idx - 1];
            ++gain_idx;
            attack_square_val = mg_value[rook - pawn];
            occ ^= square_to_bitboard(att_sq);
            attacker ^= square_to_bitboard(att_sq);
            attacker |= get_attacks(us, rook, to, occ) & color_BB[us] & (type_BB[rook] | type_BB[queen]);
        }
        else if ((bb = attacker & color_BB[us] & type_BB[queen])) {
            Square att_sq = pop_lsb(bb);
            gains[gain_idx] = attack_square_val - gains[gain_idx - 1];
            ++gain_idx;
            attack_square_val = mg_value[queen - pawn];
            occ ^= square_to_bitboard(att_sq);
            attacker ^= square_to_bitboard(att_sq);
            attacker |= get_attacks(us, bishop, to, occ) & color_BB[us] & (type_BB[bishop] | type_BB[queen]);
            attacker |= get_attacks(us, rook, to, occ) & color_BB[us] & (type_BB[rook] | type_BB[queen]);
        }
        else if ((bb = attacker & color_BB[us] & type_BB[king])) {
            if (attacker & color_BB[Color(1 - us)])
                break; // Don't allow if enemy attackers exist

            Square att_sq = pop_lsb(bb);
            gains[gain_idx] = attack_square_val - gains[gain_idx - 1];
            ++gain_idx;
            break;
        }

        // Early cutoff: if neither side can improve the result enough,
        // there is no need to continue searching deeper attackers.
        if (std::max(-gains[gain_idx - 1], gains[gain_idx]) < 0)
            break;
    }

    while (--gain_idx > 0) {
        gains[gain_idx - 1] = -std::max(-gains[gain_idx - 1], gains[gain_idx]);
    }

    return gains[0] >= 0;
}
// Pinners are their pieces, blockers are our pieces
std::pair<Bitboard, Bitboard> Position::get_pinners_and_blockers(Color us, Bitboard occ) {
    Color them = Color(1 - us);
    Square king_sq = get_lsb(color_BB[us] & type_BB[king]);
    Bitboard pinners = 0;
    Bitboard blockers = 0;

    for (PieceType piece : {bishop, rook}) {
        Bitboard curr_blockers = get_attacks(us, piece, king_sq, occ) & color_BB[us];
        Bitboard curr_occ = occ ^ curr_blockers;
        Bitboard attackers = get_attacks(us, piece, king_sq, curr_occ)
            & color_BB[them]
            & (type_BB[piece] | type_BB[queen]);

        while (attackers) {
            Square att_sq = pop_lsb(attackers);
            Bitboard blocking = line_between[king_sq][att_sq] & color_BB[us];
            if (single_pop(blocking)) {
                pinners |= square_to_bitboard(att_sq);
                blockers |= blocking;
            }
        }
    }
    return { pinners, blockers };
}
bool Position::only_pawns_left(Color us) {
    return (color_BB[us] & ~type_BB[pawn] & ~type_BB[king]) == 0;
}