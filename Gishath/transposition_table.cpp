#include "transposition_table.h"
#include <cassert>
#include <iostream>
#include <algorithm>

Bitboard TranspositionTable::get_key(Bitboard zobrist) {
    Bitboard num_buckets = table.size();
    assert((num_buckets & (num_buckets - 1)) == 0); // Ensure size is power of 2
    return zobrist & (num_buckets - 1);
}

TranspositionEntry* TranspositionTable::probe(Bitboard zobrist) {
    Bitboard key = get_key(zobrist);
    TranspositionBucket* bucket = &table[key];
    for (int i = 0; i < transposition_table_bucket_size; ++i) {
        TranspositionEntry* entry = &bucket->entries[i];
        if (entry->zobrist == zobrist && entry->flag != TranspositionFlag::empty) {
            return entry;
        }
    }
    return nullptr;
}

void TranspositionTable::insert(Bitboard zobrist, Move best_move, int16_t score, int8_t depth, TranspositionFlag flag, uint8_t age) {
    Bitboard key = get_key(zobrist);
    TranspositionBucket* bucket = &table[key];
    TranspositionEntry entry = { zobrist, best_move, score, depth, flag, age };


    // 1. Check for worse exact match
    for (int i = 0; i < transposition_table_bucket_size; ++i) {
        TranspositionEntry* curr = &bucket->entries[i];
        if (curr->flag != TranspositionFlag::empty && curr->zobrist == zobrist) {
            if (best_move.is_null()) {
                entry.best_move = curr->best_move;
            }

            const bool more_valuable = depth > curr->depth
                || (depth == curr->depth && flag == TranspositionFlag::exact)
                || age != curr->age;
            if (more_valuable) {
                *curr = entry;
            }
            else {
                curr->age = age;
                if (!best_move.is_null()) {
                    curr->best_move = best_move;
                }
            }
            return;
        }
    }

    // 2. Replace old entries
    for (int i = 0; i < transposition_table_bucket_size; ++i) {
        TranspositionEntry* curr = &bucket->entries[i];
        if (age - curr->age >= transposition_table_age_cutoff) {
            bucket->entries[i] = entry;
            return;
        }
    }

    // 3. Replace shallowest entry
    int min_depth = positive_inf;
    int min_idx = -1;
    for (int i = 0; i < transposition_table_bucket_size; ++i) {
        TranspositionEntry* curr = &bucket->entries[i];
        if (curr->depth < min_depth) {
            min_depth = curr->depth;
            min_idx = i;
        }
    }
    bucket->entries[min_idx] = entry;
}

TranspositionTable::TranspositionTable(size_t size_mb) : table((size_mb * 1024 * 1024 / sizeof(TranspositionEntry)) / transposition_table_bucket_size) {}
