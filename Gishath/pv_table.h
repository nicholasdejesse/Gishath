#pragma once
#include <array>
#include "consts.h"
#include "move.h"

class PVTable {
private:
    std::array<Move, max_ply * (max_ply + 1) / 2> table{};

public:
    void set_pv(Move m, int ply);
    Move get_pv(int ply) const;
    void clear_all();
    int length(int ply = 0) const;
    std::array<int, max_ply> pv_length{};
    const Move* line(int ply = 0) const;
};