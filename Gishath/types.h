#pragma once
#include <cstdint>

// Includes enums and generic checks related to them

typedef unsigned long long Bitboard;

constexpr int max_ply = 256; // Max ply for search

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

inline Square operator+(Square s, Direction d) { return Square(int(s) + int(d)); }
inline Square operator-(Square s, Direction d) { return Square(int(s) - int(d)); }
inline Square& operator+=(Square& s, Direction d) { return s = s + d; }
inline Square& operator-=(Square& s, Direction d) { return s = s - d; }

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

enum FileBB : Bitboard {
    file_a_bb = 0x0101010101010101,
    file_b_bb = file_a_bb << 1,
    file_c_bb = file_a_bb << 2,
    file_d_bb = file_a_bb << 3,
    file_e_bb = file_a_bb << 4,
    file_f_bb = file_a_bb << 5,
    file_g_bb = file_a_bb << 6,
    file_h_bb = file_a_bb << 7
};

enum RankBB : Bitboard {
    rank_1_bb = 0xFF,
    rank_2_bb = rank_1_bb << 8,
    rank_3_bb = rank_1_bb << 16,
    rank_4_bb = rank_1_bb << 24,
    rank_5_bb = rank_1_bb << 32,
    rank_6_bb = rank_1_bb << 40,
    rank_7_bb = rank_1_bb << 48,
    rank_8_bb = rank_1_bb << 56
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

constexpr Color color_of(Piece p) {
    return Color(p >> 3);
}

constexpr PieceType type_of(Piece p) {
    return PieceType(p & 7); // Look at bits associated with piece type
}

constexpr Square relative_square(Color c, Square sq) {
    return Square(sq ^ (c * 56)); // xor with 56 flips board
}

constexpr Direction relative_direction(Color c, Direction dir) {
    return Direction(dir * (c == white ? 1 : -1)); // flip direction for black
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

constexpr RankBB rank_bb_of(Square sq) {
    return RankBB(rank_1_bb << (8 * rank_of(sq)));
}

constexpr FileBB file_bb_of(Square sq) {
    return FileBB(file_a_bb << file_of(sq));
}