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
    for (int i = hash_len - 3; i >= 0; i -= 2) {
        if (hashes[i] == zobrist) {
            std::cout << "Found threefold" << std::endl;
            return true;
        }
    }
    return false;
}
void GameHistory::clear() {
    hash_len = 0;
}