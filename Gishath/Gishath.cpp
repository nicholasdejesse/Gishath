// Gishath.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "position.h"
#include "attacks.h"
#include "magics.h"

int main()
{
    // Precompute expensive stuff here
    init_magics();

    std::cout << "Initialization complete.\n";

    Position pos = Position();
    pos.load_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    pos.make_move(Move(a2, a3));
    std::cout << pos.to_string();
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu
