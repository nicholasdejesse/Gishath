#include "magics.h"
#include "uci.h"
#include "zobrist.h"
#include "attacks.h"

int main()
{
    // Precompute expensive stuff here
    init_magics();
    init_zobrist();

    //Position test = Position();
    //test.load_fen("2r5/1P4pk/p2p1b1p/5b1n/BB3p2/2R2p2/P1P2P2/4RK2 w - - 0 1");
    //Move capture = Move(c3, c8);
    //std::cout << test.see(capture) << std::endl;

    // Begin UCI loop
    UCIEngine engine = UCIEngine();
    engine.loop();
}