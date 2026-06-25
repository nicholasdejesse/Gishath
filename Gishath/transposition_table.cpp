#include "transposition_table.h"
#include <cassert>
#include <iostream>
#include <algorithm>

uint32_t TranspositionTable::get_key(Bitboard zobrist) {
    assert(((transposition_table_size / transposition_table_bucket_size) & (transposition_table_size / transposition_table_bucket_size - 1)) == 0);
    return zobrist & ((transposition_table_size / transposition_table_bucket_size) - 1);
}

TranspositionEntry* TranspositionTable::probe(Bitboard zobrist) {
    //++probe_count;
    uint32_t key = get_key(zobrist);
    TranspositionBucket* bucket = &table[key];
    for (int i = 0; i < transposition_table_bucket_size; ++i) {
        TranspositionEntry* entry = &bucket->entries[i];
        if (entry->zobrist == zobrist && entry->flag != TranspositionFlag::empty) {
            //++hit_count;
            return entry;
        }
    }
    return nullptr;
}

void TranspositionTable::insert(Bitboard zobrist, Move best_move, int16_t score, int8_t depth, TranspositionFlag flag) {
    uint32_t key = get_key(zobrist);
    TranspositionBucket* bucket = &table[key];
    if(bucket->entries[0].flag == TranspositionFlag::empty || bucket->entries[0].depth <= depth)
        bucket->entries[0] = { zobrist, best_move, score, depth, flag };
    bucket->entries[1] = { zobrist, best_move, score, depth, flag };
}

TranspositionBucket::TranspositionBucket() : entries(transposition_table_bucket_size, TranspositionEntry(0, 0, 0, 0, TranspositionFlag::empty)) {}
TranspositionTable::TranspositionTable() : table(transposition_table_size / transposition_table_bucket_size, TranspositionBucket()) {}