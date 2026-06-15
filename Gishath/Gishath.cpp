#include <iostream>
#include "position.h"
#include "magics.h"
#include "movegen.h"
#include "uci.h"

int main()
{
    // Precompute expensive stuff here
    init_magics();

    // Begin UCI loop
    UCIEngine engine = UCIEngine();
    engine.loop();
}