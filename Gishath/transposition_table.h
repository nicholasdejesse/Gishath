#pragma once
#include <vector>
#include "consts.h"
#include "types.h"
#include "move.h"

enum class TranspositionFlag {
    exact,
    upper_bound,
    lower_bound,
    empty // No entry
};

struct TranspositionEntry {
    Bitboard zobrist = 0;
    Move best_move = 0;
    int32_t score = 0;
    int8_t depth = 0;
    TranspositionFlag flag = TranspositionFlag::exact;
};

class TranspositionTable {
private:
    std::vector<TranspositionEntry> table;
    uint32_t get_key(Bitboard zobrist);

public:
    bool contains(Bitboard zobrist);
    TranspositionEntry get(Bitboard zobrist);
    void insert(TranspositionEntry entry);
    //int capacity = 0;
    //int collisions = 0;

    TranspositionTable();
};