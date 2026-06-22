#include "transposition_table.h"

uint32_t TranspositionTable::get_key(Bitboard zobrist) {
    // Use only bottom half of zobrist for hash
    return zobrist & Bitboard(0xffffffff);
}

bool TranspositionTable::contains(Bitboard zobrist) {
    uint32_t key = get_key(zobrist);
    return table.contains(key);
}

TranspositionEntry TranspositionTable::get(Bitboard zobrist) {
    uint32_t key = get_key(zobrist);
    return table.at(key);
}

void TranspositionTable::insert(TranspositionEntry entry) {
    uint32_t key = get_key(entry.zobrist);
    table.try_emplace(key, entry.zobrist, entry.best_move, entry.score, entry.depth, entry.flag); // Using always replace scheme
}

TranspositionTable::TranspositionTable(size_t table_size) : table{} {
    table.reserve(table_size);
}