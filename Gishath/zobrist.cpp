#include "zobrist.h"
#include <random>

void init_zobrist() {
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<Bitboard> dis;

    for (int p = 0; p < piece_num; ++p) {
        for (int sq = 0; sq < square_num; ++sq) {
            zobrist_pieces[p][sq] = dis(gen);
        }
    }
    zobrist_black_to_move = dis(gen);
    for (int cas = 0; cas < 16; ++cas) {
        zobrist_castling[cas] = dis(gen);
    }
    for (int f = 0; f < 8; ++f) {
        zobrist_en_passant_file[f] = dis(gen);
    }
}