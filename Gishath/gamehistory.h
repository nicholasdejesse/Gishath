#pragma once
#include <array>
#include "types.h"
#include "consts.h"

// Stores history of positions as Zobrists
class GameHistory {
private:
    std::array<Bitboard, max_ply> hashes{};
    int hash_len = 0;

public:
    void push(Bitboard zobrist, bool reversible);
    void pop();
    bool repeated(Bitboard zobrist);
    void clear();
};