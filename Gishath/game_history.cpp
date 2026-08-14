#include "game_history.h"

GameHistory::GameHistory() {
    hashes.reserve(initial_history_length);
    is_reversible.reserve(initial_history_length);
}

void GameHistory::push(Bitboard zobrist, bool reversible) {
    hashes.emplace_back(zobrist);
    is_reversible.emplace_back(reversible);
}

void GameHistory::pop() {
    hashes.pop_back();
    is_reversible.pop_back();
}

bool GameHistory::repeated(Bitboard zobrist, int search_ply) {
    int prior_occurrences = 0;
    const int current_idx = static_cast<int>(hashes.size()) - 1;

    for (int i = static_cast<int>(hashes.size()) - 3; i >= 0; i -= 2) {
        if (!is_reversible[i] || !is_reversible[i + 1]) {
            return false;
        }
        if (hashes[i] == zobrist) {
            // A repetition made within the search tree can be treated as a draw
            // immediately to prevent the search from following cycles forever.
            // Matches from the played game require two prior occurrences, as
            // required by the threefold-repetition rule.
            if (current_idx - i <= search_ply || ++prior_occurrences >= 2) {
                return true;
            }
        }
    }
    return false;
}
