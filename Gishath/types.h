#pragma once
#include <cstdint>
#include <limits>

// Includes enums and generic checks related to them

typedef unsigned long long Bitboard;

constexpr int max_ply = 256; // Max ply for search
constexpr int move_list_size = 256;

constexpr int checkmate_score = 999;
constexpr int draw_score = 0;
constexpr int min_eval_score = std::numeric_limits<int>::min();

enum Square : uint8_t {
    a1, b1, c1, d1, e1, f1, g1, h1,
    a2, b2, c2, d2, e2, f2, g2, h2,
    a3, b3, c3, d3, e3, f3, g3, h3,
    a4, b4, c4, d4, e4, f4, g4, h4,
    a5, b5, c5, d5, e5, f5, g5, h5,
    a6, b6, c6, d6, e6, f6, g6, h6,
    a7, b7, c7, d7, e7, f7, g7, h7,
    a8, b8, c8, d8, e8, f8, g8, h8,
    square_num = 64
};

enum Direction : int8_t {
    north = 8,
    east = 1,
    south = -1 * north,
    west = -1 * east,
    north_east = north + east,
    north_west = north + west,
    south_east = south + east,
    south_west = south + west
};

constexpr Square operator+(Square s, Direction d) { return Square(int(s) + int(d)); }
constexpr Square operator-(Square s, Direction d) { return Square(int(s) - int(d)); }
constexpr Square& operator+=(Square& s, Direction d) { return s = s + d; }
constexpr Square& operator-=(Square& s, Direction d) { return s = s - d; }

enum Color : uint8_t {
    white,
    black,
    color_num = 2
};

enum File : uint8_t {
    file_a,
    file_b,
    file_c,
    file_d,
    file_e,
    file_f,
    file_g,
    file_h
};

enum Rank : uint8_t {
    rank_1,
    rank_2,
    rank_3,
    rank_4,
    rank_5,
    rank_6,
    rank_7,
    rank_8
};

enum CastleRights : uint8_t {
    WK = 1,
    WQ = 2,
    BK = 4,
    BQ = 8
};

// Based on stockfish move encoding
// Bits 0-5: to square (0..63)
// Bits 6-11: from square (0..63)
// Bits 12-13: promotion piece type (0..3) (knight, bishop, rook, queen)
// Bit 14-15: special move flag (0..3) (quiet, promotion, castle, en passant)

enum MoveType : uint16_t {
    normal,
    promotion = 1 << 14,
    en_passant = 2 << 14,
    castle = 3 << 14,
};

enum PieceType : uint8_t {
    no_piece_type,
    pawn,
    knight,
    bishop,
    rook,
    queen,
    king,
    all_pieces = 0, // Stores every piece
    piece_type_num = 7
};

enum Piece : uint8_t {
    no_piece,
    white_pawn,
    white_knight,
    white_bishop,
    white_rook,
    white_queen,
    white_king,
    black_pawn = white_pawn + 8,
    black_knight,
    black_bishop,
    black_rook,
    black_queen,
    black_king,
};

constexpr Piece make_piece(Color c, PieceType pt) {
    return Piece((c << 3) | pt);
}

constexpr Square make_square(int rank, int file) {
    return Square(8 * rank + file);
}

constexpr Color color_of(Piece p) {
    return Color(p >> 3);
}

constexpr PieceType piecetype_of(Piece p) {
    return PieceType(p & 7); // Look at bits associated with piece type
}
constexpr Bitboard relative_bb(Color us, Bitboard bb) {
    return us == white ? bb : ((bb << 56)) |
        ((bb << 40) & Bitboard(0x00ff000000000000)) |
        ((bb << 24) & Bitboard(0x0000ff0000000000)) |
        ((bb << 8) & Bitboard(0x000000ff00000000)) |
        ((bb >> 8) & Bitboard(0x00000000ff000000)) |
        ((bb >> 24) & Bitboard(0x0000000000ff0000)) |
        ((bb >> 40) & Bitboard(0x000000000000ff00)) |
        ((bb >> 56));
}
constexpr Square relative_square(Color us, Square sq) {
    return Square(sq ^ (us * 56)); // xor with 56 flips board
}

constexpr Direction relative_direction(Color c, Direction dir) {
    if (c == white) return dir;
    switch (dir) {
    case north:      return south;
    case south:      return north;
    case north_east: return south_east;
    case north_west: return south_west;
    case south_east: return north_east;
    case south_west: return north_west;
    default:         return dir;
    }
}

constexpr Bitboard square_to_bitboard(Square sq) {
    return Bitboard(1) << sq;
}

constexpr Rank rank_of(Square sq) {
    return Rank(sq / 8);
}

constexpr File file_of(Square sq) {
    return File(sq & 7);
}

constexpr Bitboard rank_bb_of(Square sq) {
    return Bitboard(0xFF) << (8 * rank_of(sq));
}

constexpr Bitboard file_bb_of(Square sq) {
    return Bitboard(0x0101010101010101) << file_of(sq);
}

constexpr Bitboard rank_bb_of(Rank r) {
    return Bitboard(0xFF) << (8 * r);
}

constexpr Bitboard file_bb_of(File f) {
    return Bitboard(0x0101010101010101) << f;
}

constexpr Rank relative_rank(Color c, Rank r) {
    return Rank(r ^ (7 * c));
}

constexpr Square operator+(Square s, int i) { return Square(uint8_t(s) + i); }
constexpr Square operator-(Square s, int i) { return Square(uint8_t(s) - i); }
