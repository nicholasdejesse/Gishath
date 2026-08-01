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
            std::cout << "option name Hash type spin default " << uci_hash_default_size << " min " << uci_hash_min_size << " max " << uci_hash_max_size << std::endl;
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
            if (search_thread.joinable()) {
                search_thread.join();
            }
            if (input_size > 1 && input[1] == "perft") {
                int depth = std::stoi(input[2]);
                run_perft(pos, depth);
            }
            else {
                stop_search.store(false);
                int max_depth = 9999;
                if (input_size >= 3 && input[1] == "depth") {
                    max_depth = std::stoi(input[2]);
                }
                unsigned long wtime = 5000 * 20;
                unsigned long btime = 5000 * 20;
                unsigned long winc = 0;
                unsigned long binc = 0;
                if (input_size > 1 && input[1] == "infinite") {
                    wtime = std::numeric_limits<unsigned long>::max();
                    btime = std::numeric_limits<unsigned long>::max();
                }
                // This logic is a fragile, should switch later
                if (input_size >= 5 && input[1] == "wtime" && input[3] == "btime") {
                    wtime = std::stoul(input[2]);
                    btime = std::stoul(input[4]);
                    if (input_size >= 9 && input[5] == "winc" && input[7] == "binc") {
                        winc = std::stoul(input[6]);
                        binc = std::stoul(input[8]);
                    }
                }
                unsigned long ms_to_search = get_time_for_move(wtime, btime, winc, binc);
                Position search_pos = pos;

                search_thread = std::thread([this, search_pos, ms_to_search, max_depth]() mutable {
                    Move best_move = find_best_move(search_pos, ms_to_search, *this, max_depth);

                    if (!stop_search.load())
                        std::cout << "bestmove " << best_move.to_string() << std::endl;
                });
            }
        }
        else if (input[0] == "stop") {
            stop_search.store(true);
            if (search_thread.joinable()) {
                search_thread.join();
            }
        }
        else if (input[0] == "setoption") {
            if (input_size >= 5 && input[2] == "Hash") {
                hash_size = static_cast<size_t>(std::stoull(input[4]));
                transposition_table = TranspositionTable(hash_size);
            }
        }
        else if (input[0] == "quit") {
            stop_search.store(true);
            if (search_thread.joinable()) {
                search_thread.join();
            }
            break;
        }
    }
}

void UCIEngine::new_game() {
    transposition_table = TranspositionTable(hash_size);
    for (auto& row : killers) {
        row.fill(Move(0));
    }
    for (auto& color_row : history_table) {
        for (auto& from_row : color_row) {
            from_row.fill(0);
        }
    }
    for (auto& row : countermove_table) {
        row.fill(Move(0));
    }

    pv_table.clear_all();
}
// Called when "position ..." is passed in, does not reset anything related to current game (i.e. transposition table)
void UCIEngine::reset_position() {
    pos = Position();
}

void UCIEngine::write_search_info(int depth, int seldepth, int score, int nodes_visited, int time_elapsed_ms) {
    std::string seldepth_str = "";
    if (depth != seldepth) {
        seldepth_str += " seldepth " + std::to_string(seldepth);
    }

    std::string score_str = "";
    if (score > checkmate_score - max_ply) {
        score_str += " score mate " + std::to_string(static_cast<int>(std::round(static_cast<double>(checkmate_score - score) / 2)));
    }
    else if (score < -checkmate_score + max_ply) {
        score_str += " score mate " + std::to_string(static_cast<int>(std::round(static_cast<double>(-checkmate_score - score)) / 2));
    }
    else {
        score_str += " score cp " + std::to_string(score);
    }

    // Avoid division by zero
    time_elapsed_ms = std::max(1, time_elapsed_ms);
    // Divide first to avoid integer overflow
    std::string nps = std::to_string(static_cast<int>((static_cast<double>(nodes_visited) / time_elapsed_ms) * 1000));
    std::cout << "info depth " << depth << seldepth_str << score_str << " nodes " << nodes_visited << " nps " << nps;
    std::cout << " pv";
    for (int i = 0; i < pv_table.length(0); ++i) {
        std::cout << ' ' << pv_table.line(0)[i].to_string();
    }
    std::cout << std::endl;
}

unsigned long UCIEngine::get_time_for_move(unsigned long wtime, unsigned long btime, unsigned long winc, unsigned long binc) {
    // Just divide remaining time by 20
    unsigned long our_time = pos.side_to_move() == white ? wtime : btime;
    unsigned long our_inc = pos.side_to_move() == white ? winc : binc;
    unsigned long time_allotted = our_time / 20 + our_inc / 2;
    if (time_allotted > 0) {
        return time_allotted;
    }
    return our_time;
}

UCIEngine::UCIEngine() : transposition_table(uci_hash_default_size), killers{}, history_table{} {}