#pragma once
#include <array>
#include "types.h"
#include "consts.h"

// Stores history of positions as Zobrists
class GameHistory {
private:
    std::array<Bitboard, history_length> hashes{};
    int last_reversible = 0;
    int idx = 0;

public:
    void push(Bitboard zobrist, bool reversible);
    void pop();
    bool repeated(Bitboard zobrist);
    void clear();
};