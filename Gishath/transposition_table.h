#pragma once
#include <vector>
#include "consts.h"
#include "types.h"
#include "move.h"

enum class TranspositionFlag : uint8_t {
    exact,
    upper_bound,
    lower_bound,
    empty // No entry
};

struct TranspositionEntry {
    Bitboard zobrist = 0;
    Move best_move = 0;
    int16_t score = 0;
    int8_t depth = 0;
    TranspositionFlag flag = TranspositionFlag::empty;
    uint8_t age = 0;
};

struct TranspositionBucket {
    std::vector<TranspositionEntry> entries;

    TranspositionBucket();
};

class TranspositionTable {
private:
    std::vector<TranspositionBucket> table;
    Bitboard get_key(Bitboard zobrist);

public:
    TranspositionEntry* probe(Bitboard zobrist);
    void insert(Bitboard zobrist, Move best_move, int16_t score, int8_t depth, TranspositionFlag flag, uint8_t age);

    TranspositionTable(size_t size_mb);
};