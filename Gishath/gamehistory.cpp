#include "gamehistory.h"
#include <iostream>

void GameHistory::push(Bitboard zobrist, bool reversible) {
    if (!reversible) last_reversible = idx;
    hashes[idx] = zobrist;
    ++idx;
}

void GameHistory::pop() {
    --idx;
}

bool GameHistory::repeated(Bitboard zobrist) {
    for (int i = idx - 3; i >= last_reversible; i -= 2) {
        if (hashes[i] == zobrist) {
            return true;
        }
    }
    return false;
}
void GameHistory::clear() {
    last_reversible = 0;
    idx = 0;
}