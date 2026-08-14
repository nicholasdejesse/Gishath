#pragma once
#include <array>
#include <vector>
#include "types.h"
#include "consts.h"

// Stores history of positions as Zobrists
class GameHistory {
private:
    std::vector<Bitboard> hashes{};
    std::vector<bool> is_reversible{};
    int idx = 0;

public:
    void push(Bitboard zobrist, bool reversible);
    void pop();
    bool repeated(Bitboard zobrist, int search_ply);

    GameHistory();
};
