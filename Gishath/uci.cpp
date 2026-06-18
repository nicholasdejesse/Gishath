#include "uci.h"
#include "perft.h"

void UCIEngine::loop() {
    std::string line_in = "";
    while (std::getline(std::cin, line_in)) {
        std::istringstream in_stream(line_in);
        std::vector<std::string> input((std::istream_iterator<std::string>(in_stream)), std::istream_iterator<std::string>());

        size_t input_size = input.size();

        if (input_size == 0) continue;
        
        if (input[0] == "uci") {
            std::cout << "id name Gishath" << std::endl;
            std::cout << "id author Nicholas DeJesse" << std::endl;
            std::cout << "uciok" << std::endl;
        }
        else if (input[0] == "isready") {
            std::cout << "readyok" << std::endl;
        }
        else if (input[0] == "ucinewgame") {
            new_game();
        }
        else if (input[0] == "position") {
            reset_position();
            int i = 2;
            if (input[1] == "startpos") {
                pos.load_fen(start_fen);
            }
            else if (input[1] == "fen") {
                std::string fen_str = "";
                while (i < input_size && input[i] != "moves") {
                    fen_str += input[i];
                    if ((i + 1) < input_size && input[i + 1] != "moves") {
                        fen_str += " ";
                    }
                    ++i;
                }
                pos.load_fen(fen_str);
            }
            if (i < input_size && input[i] == "moves") {
                // Load moves
                ++i;
                while (i < input_size) {
                    pos.make_move(Move(input[i], pos));
                    ++i;
                }
            }
        }
        else if (input[0] == "go") {
            if (input_size > 1 && input[1] == "perft") {
                int depth = std::stoi(input[2]);
                run_perft(pos, depth);
            }
            else {
                Move best_move = find_best_move(pos, 3000);
                std::cout << "bestmove " << best_move.to_string() << std::endl;
            }
        }
    }
}

void UCIEngine::new_game() {
    // Need to implement this
}
// Called when "position ..." is passed in, does not reset anything related to current game (i.e. transposition tables)
void UCIEngine::reset_position() {
    pos = Position();
}