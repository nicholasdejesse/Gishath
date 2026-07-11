#include "pv_table.h"
#include <algorithm>

size_t triangle_pv_index(int ply) {
    return static_cast<size_t>(ply) * max_ply - static_cast<size_t>((ply * (ply - 1)) / 2);
}

void PVTable::set_pv(Move m, int ply) {
    if (ply < 0 || ply >= max_ply) return;

    size_t dst = triangle_pv_index(ply);
    table[dst] = m;

    int child_len = 0;
    if (ply + 1 < max_ply) {
        child_len = pv_length[ply + 1];
        size_t src = triangle_pv_index(ply + 1);

        std::copy_n(
            table.begin() + src,
            child_len,
            table.begin() + dst + 1
        );
    }

    pv_length[ply] = child_len + 1;
}

void PVTable::clear_all() {
    std::fill(table.begin(), table.end(), Move(0));
    std::fill(pv_length.begin(), pv_length.end(), 0);
}

int PVTable::length(int ply) const {
    return pv_length[ply];
}

const Move* PVTable::line(int ply) const {
    return &table[triangle_pv_index(ply)];
}
