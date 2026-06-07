// Gishath.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "position.h"

int main()
{
    // TODO: Precompute any expensive stuff here (e.g. attack bitboards)

    Position pos = Position();
    pos.load_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    pos.make_move(Move(a2, a3, 0));
    std::cout << pos.to_string();
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu
