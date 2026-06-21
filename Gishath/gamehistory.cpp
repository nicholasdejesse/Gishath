#include "gamehistory.h"

GameHistory::GameHistory() {
    hashes.reserve(initial_history_length);
    is_reversible.reserve(initial_history_length);
}

void GameHistory::push(Bitboard zobrist, bool reversible) {
    hashes.push_back(zobrist);
    is_reversible.push_back(reversible);
}

void GameHistory::pop() {
    hashes.pop_back();
    is_reversible.pop_back();
}

bool GameHistory::repeated(Bitboard zobrist) {
    for (int i = hashes.size() - 3; i >= 0; i -= 2) {
        if (!is_reversible[i] || !is_reversible[i + 1]) {
            return false;
        }
        if (hashes[i] == zobrist) {
            return true;
        }
    }
    return false;
}