#include <iostream>
#include "position.h"
#include "magics.h"
#include "uci.h"
#include "zobrist.h"

int main()
{
    // Precompute expensive stuff here
    init_magics();
    init_zobrist();

    // Begin UCI loop
    UCIEngine engine = UCIEngine();
    engine.loop();
}