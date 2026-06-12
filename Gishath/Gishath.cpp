// Gishath.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "position.h"
#include "magics.h"
#include "movegen.h"

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
    pos.load_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    //pos.make_move(Move(e2, e4));
    //pos.make_move(Move(d7, d5));
    //pos.make_move(Move(e4, e5));
    //pos.make_move(Move(f7, f5));
    //pos.make_move(Move(e5, f6, en_passant));
    //pos.unmake_move(Move(e5, f6, en_passant));
    //pos.unmake_move(Move(e4, d5));
    //pos.unmake_move(Move(d7, d5));
    //pos.unmake_move(Move(e2, e4));
    std::cout << pos.to_string();

    MoveList moves;
    generate(pos, moves);
    for (int i = 0; i < moves.count; ++i) {
        std::cout << moves.moves[i] << " ";
    }
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu
