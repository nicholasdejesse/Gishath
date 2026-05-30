#pragma once
#include "board.h"
#include <cstdint>

enum class CastleRights : uint8_t {
    WK = 1,
    WQ = 2,
    BK = 4,
    BQ = 8
};

class Position {
public:
    Board board_state;
    bool white_to_move;
    CastleRights castle_rights;
    uint8_t halfmove_clock;
    uint16_t fullmove_clock;
    int8_t en_passant_file;
};