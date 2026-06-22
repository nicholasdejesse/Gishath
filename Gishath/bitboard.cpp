#include "bitboard.h"
#include <cassert>
#include <bit>

Square pop_lsb(Bitboard& bb) {
    if (!bb) return Square(64);
    // Try to use built in if possible
#if defined(__GNUC__) || defined(__clang__)
    int idx = __builtin_ctzll(bb);
#elif defined(_MSC_VER)
    unsigned long idx;
    _BitScanForward64(&idx, bb);
#else
    const int index64[64] = {
        0,  1, 48,  2, 57, 49, 28,  3,
       61, 58, 50, 42, 38, 29, 17,  4,
       62, 55, 59, 36, 53, 51, 43, 22,
       45, 39, 33, 30, 24, 18, 12,  5,
       63, 47, 56, 27, 60, 41, 37, 16,
       54, 35, 52, 21, 44, 32, 23, 11,
       46, 26, 40, 15, 34, 20, 31, 10,
       25, 14, 19,  9, 13,  8,  7,  6
    };
    int idx = index64[((bb & -bb) * 0x03f79d71b4cb0a89) >> 58];
#endif
    bb &= bb - 1;
    return Square(idx);
}

Square get_lsb(Bitboard bb) {
    if (!bb) return Square(64);
    // Try to use built in if possible
#if defined(__GNUC__) || defined(__clang__)
    int idx = __builtin_ctzll(bb);
#elif defined(_MSC_VER)
    unsigned long idx;
    _BitScanForward64(&idx, bb);
#else
    const int index64[64] = {
        0,  1, 48,  2, 57, 49, 28,  3,
       61, 58, 50, 42, 38, 29, 17,  4,
       62, 55, 59, 36, 53, 51, 43, 22,
       45, 39, 33, 30, 24, 18, 12,  5,
       63, 47, 56, 27, 60, 41, 37, 16,
       54, 35, 52, 21, 44, 32, 23, 11,
       46, 26, 40, 15, 34, 20, 31, 10,
       25, 14, 19,  9, 13,  8,  7,  6
    };
    int idx = index64[((bb & -bb) * 0x03f79d71b4cb0a89) >> 58];
#endif
    return Square(idx);
}