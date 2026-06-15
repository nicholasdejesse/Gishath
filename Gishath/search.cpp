#include "search.h"

Move find_best_move(Position& pos) {
    MoveList ml;
    generate(pos, ml);

    for (int i = 0; i < ml.count; ++i) {
        pos.make_move(ml.moves[i]);
        if (!pos.legal()) {
            pos.unmake_move(ml.moves[i]);
            continue;
        }
        return ml.moves[i];
    }
}