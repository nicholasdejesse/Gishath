#include "gamehistory.h"
#include <iostream>

void GameHistory::push(Bitboard zobrist, bool reversible) {
    hash_len_history[hash_len] = hash_len;
    if (!reversible) hash_len = 0;
    hashes[hash_len] = zobrist;
    ++hash_len;
    ++hash_history_idx;
}

void GameHistory::pop() {
    if (hash_history_idx > 0) {
        --hash_history_idx;
        hash_len = hash_len_history[hash_history_idx];
    }
}

bool GameHistory::repeated(Bitboard zobrist) {
    for (int i = hash_len - 3; i >= 0; i -= 2) {
        if (hashes[i] == zobrist) {
            return true;
        }
    }
    return false;
}
void GameHistory::clear() {
    hash_len = 0;
    hash_history_idx = 0;
}