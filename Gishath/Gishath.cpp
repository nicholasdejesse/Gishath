#include <iostream>
#include "position.h"
#include "magics.h"
#include "movegen.h"
#include "perft.h"

int main()
{
    // Precompute expensive stuff here
    init_magics();

    std::cout << "Initialization complete.\n";

    // Magic tests
    //Bitboard occ = 61581912178704;
    //Square sq = a1;
    //std::cout << get_rook_attacks(sq, occ) << "\n";
    //std::cout << get_bishop_attacks(sq, occ) << "\n";

    Position pos = Position();
    //pos.load_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    pos.load_fen("rnbq1k1r/1p1Pbppp/p1p5/8/2B5/P7/1PP1NnPP/RNBQK2R w KQ - 0 1");
    std::cout << pos.to_string();

    run_perft(pos, 2);

    //std::cout << "Board after perft: \n" << pos.to_string();
}