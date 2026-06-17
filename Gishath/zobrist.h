#pragma once
#include <array>
#include "types.h"

inline std::array<std::array<Bitboard, square_num>, piece_num> zobrist_pieces;
inline Bitboard zobrist_black_to_move;
inline std::array<Bitboard, 16> zobrist_castling; // Store 2^4 combinations
inline std::array<Bitboard, 8> zobrist_en_passant_file;

void init_zobrist();