#include "transposition_table.h"
#include <cassert>
#include <iostream>
#include <algorithm>

uint32_t TranspositionTable::get_key(Bitboard zobrist) {
    // Use only bottom half of zobrist for hash
    assert((transposition_table_size & (transposition_table_size - 1)) == 0);
    return zobrist & (transposition_table_size - 1);
}

bool TranspositionTable::contains(Bitboard zobrist) {
    uint32_t key = get_key(zobrist);
    assert(key < table.size());
    return table[key].zobrist == zobrist && table[key].flag != TranspositionFlag::empty;
}

TranspositionEntry TranspositionTable::get(Bitboard zobrist) {
    uint32_t key = get_key(zobrist);
    return table[key];
}

void TranspositionTable::insert(TranspositionEntry entry) {
    uint32_t key = get_key(entry.zobrist);
    //if (table[key].flag == TranspositionFlag::empty) {
    //    ++capacity;
    //}
    //else {
    //    if (table[key].zobrist != entry.zobrist) ++collisions;
    //}
    if (table[key].flag == TranspositionFlag::empty || table[key].depth <= entry.depth)
        table[key] = entry; // Using depth replacement scheme
}

TranspositionTable::TranspositionTable() {
    table = std::vector<TranspositionEntry>(transposition_table_size, TranspositionEntry(0, 0, 0, 0, TranspositionFlag::empty));
    //capacity = 0;
    //collisions = 0;
}