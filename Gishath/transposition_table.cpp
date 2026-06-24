#include "transposition_table.h"
#include <cassert>
#include <iostream>
#include <algorithm>

uint32_t TranspositionTable::get_key(Bitboard zobrist) {
    assert((transposition_table_size & (transposition_table_size - 1)) == 0);
    return zobrist & (transposition_table_size - 1);
}

TranspositionEntry* TranspositionTable::probe(Bitboard zobrist) {
    uint32_t key = get_key(zobrist);
    TranspositionEntry* entry = &table[key];
    if (entry->zobrist == zobrist && entry->flag != TranspositionFlag::empty) {
        return entry;
    }
    return nullptr;
}

void TranspositionTable::insert(Bitboard zobrist, Move best_move, int16_t score, int8_t depth, TranspositionFlag flag) {
    uint32_t key = get_key(zobrist);
    if (table[key].flag == TranspositionFlag::empty || table[key].depth <= depth) {
        table[key] = { zobrist, best_move, score, depth, flag }; // Using depth replacement scheme
    }
}

TranspositionTable::TranspositionTable() : table(transposition_table_size, TranspositionEntry(0, 0, 0, 0, TranspositionFlag::empty)) {}