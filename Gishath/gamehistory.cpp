#include "gamehistory.h"
#include <iostream>

void GameHistory::push(Bitboard zobrist, bool reversible) {
    if (!reversible) hash_len = 0;
    hashes[hash_len] = zobrist;
    ++hash_len;
}

void GameHistory::pop() {
    if (hash_len > 0) --hash_len;
}

bool GameHistory::repeated(Bitboard zobrist) {
    for (int i = hash_len - 1; i > 0; i -= 2) {
        if (hashes[i] == zobrist) {
            return true;
        }
    }
    return false;
}