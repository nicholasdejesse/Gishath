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
    int16_t score = 0;
    int8_t depth = 0;
    TranspositionFlag flag = TranspositionFlag::empty;
};

struct TranspositionBucket {
    std::vector<TranspositionEntry> entries;

    TranspositionBucket();
};

class TranspositionTable {
private:
    std::vector<TranspositionBucket> table;
    uint32_t get_key(Bitboard zobrist);

public:
    TranspositionEntry* probe(Bitboard zobrist);
    void insert(Bitboard zobrist, Move best_move, int16_t score, int8_t depth, TranspositionFlag flag);
    //int hit_count = 0;
    //int probe_count = 0;
    //int insert_empty_count = 0;
    //int replace_count = 0;

    TranspositionTable();
};