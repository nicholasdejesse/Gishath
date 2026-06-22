#pragma once
#include <unordered_map>
#include "types.h"
#include "move.h"

enum class TranspositionFlag {
    exact,
    upper_bound,
    lower_bound
};

struct TranspositionEntry {
    Bitboard zobrist;
    Move best_move;
    int32_t score;
    int8_t depth;
    TranspositionFlag flag;

    TranspositionEntry(Bitboard zobrist, Move best_move, int32_t score, int8_t depth, TranspositionFlag flag) : zobrist(zobrist), best_move(best_move), score(score), depth(depth), flag(flag) {}
};

class TranspositionTable {
private:
    std::unordered_map<uint32_t, TranspositionEntry> table;
    uint32_t get_key(Bitboard zobrist);

public:
    bool contains(Bitboard zobrist);
    TranspositionEntry get(Bitboard zobrist);
    void insert(TranspositionEntry entry);

    TranspositionTable(size_t table_size);
};