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
                    pos.make_move(Move(input[i], pos), true);
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
                int max_depth = 9999;
                if (input_size >= 3 && input[1] == "depth") {
                    max_depth = std::stoi(input[2]);
                }
                int wtime = 10000 * 20;
                int btime = 10000 * 20;
                // This logic is a bit frail, should switch later
                if (input_size > 5 && input[1] == "wtime" && input[3] == "btime") {
                    wtime = std::stoi(input[2]);
                    btime = std::stoi(input[4]);
                }
                int ms_to_search = get_time_for_move(wtime, btime);
                Move best_move = find_best_move(pos, ms_to_search, *this, max_depth);
                std::cout << "bestmove " << best_move.to_string() << std::endl;
            }
        }
        else if (input[0] == "quit") {
            break;
        }
    }
}

void UCIEngine::new_game() {
    transposition_table = TranspositionTable(transposition_table_size);
}
// Called when "position ..." is passed in, does not reset anything related to current game (i.e. transposition table)
void UCIEngine::reset_position() {
    pos = Position();
}

void UCIEngine::write_search_info(int depth, int score, int nodes_visited) {
    std::cout << "info depth " << depth << " score cp " << score << " nodes " << nodes_visited << std::endl;
}

int UCIEngine::get_time_for_move(int wtime, int btime) {
    // Just divide remaining time by 20
    int our_time = pos.side_to_move() == white ? wtime : btime;
    int time_allotted = our_time / 20;
    if (time_allotted > 0) {
        return time_allotted;
    }
    return our_time;
}

UCIEngine::UCIEngine() : transposition_table(transposition_table_size) {}