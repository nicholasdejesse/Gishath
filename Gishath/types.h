#pragma once
#include <cstdint>

// Includes enums and generic checks related to them

typedef unsigned long long Bitboard;
constexpr int SQUARE_NUM = 64;


enum Square : uint8_t {
    a1, a2, a3, a4, a5, a6, a7, a8,
    b1, b2, b3, b4, b5, b6, b7, b8,
    c1, c2, c3, c4, c5, c6, c7, c8,
    d1, d2, d3, d4, d5, d6, d7, d8,
    e1, e2, e3, e4, e5, e6, e7, e8,
    f1, f2, f3, f4, f5, f6, f7, f8,
    g1, g2, g3, g4, g5, g6, g7, g8,
    h1, h2, h3, h4, h5, h6, h7, h8
};

enum CastleRights : uint8_t {
    WK = 1,
    WQ = 2,
    BK = 4,
    BQ = 8
};

enum PieceType : uint8_t {
    no_piece_type = -1,
    pawn = 0,
    knight = 1,
    bishop = 2,
    rook = 3,
    queen = 4,
    king = 5,
    piece_type_num = 6
};

enum Piece : uint8_t {
    no_piece = -1,
    white_pawn = 0,
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

enum Color : uint8_t {
    white,
    black,
    color_num = 2
};

constexpr Color color_of(Piece p) {
    return Color(p >> 3);
}

constexpr PieceType type_of(Piece p) {
    return PieceType(p & 7); // Look at bits associated with piece type
}

constexpr Square relative_square(Color c, Square sq) {
    return Square(sq ^ (c * 56)); // xor with 56 flips board
}