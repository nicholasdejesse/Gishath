#pragma once
#include <array>
#include "types.h"
#include "consts.h"

// Stores history of positions as Zobrists
class GameHistory {
private:
    std::array<Bitboard, history_length> hashes{};
    std::array<int, history_length> hash_len_history{};
    int hash_history_idx = 0;
    int hash_len = 0;

public:
    void push(Bitboard zobrist, bool reversible);
    void pop();
    bool repeated(Bitboard zobrist);
    void clear();
};