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
    pos.load_fen("r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10");
    std::cout << pos.to_string();

    run_perft(pos, 5);

    //std::cout << "Board after perft: \n" << pos.to_string();
}