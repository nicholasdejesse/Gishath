// Lightweight script for generating a bunch of NNUE data

#include <iostream>
#include <fstream>
#include <random>
#include <vector>
#include <algorithm>
#include <filesystem>
#include <stdexcept>
#include "Gishath/position.h"
#include "Gishath/bitboard.h"
#include "Gishath/uci.h"
#include "Gishath/eval.h"
#include "Gishath/magics.h"
#include "Gishath/zobrist.h"
#include <chrono>

// Settings
constexpr int soft_nodes_per_position = 5000;
constexpr int num_positions = 5'000'000; // This is a soft limit, final game will not be truncated
constexpr int num_random_plies = 10; // Number of random plies to play at start of game
constexpr int transposition_table_size = 128; // MB
constexpr int progress_bar_width = 50;
constexpr bool resume_existing_data = true;
std::string viriformat_output_path = "./training_1.viri";

constexpr std::streamsize initial_record_size = 32;
constexpr std::streamsize move_record_size = 4;

std::ofstream out;
int positions_written = 0;

void prepare_output_file() {
    positions_written = 0;

    if (!resume_existing_data) {
        out.open(viriformat_output_path, std::ios::binary | std::ios::trunc);
        if (!out) {
            throw std::runtime_error("Could not open output file: " + viriformat_output_path);
        }
        std::cout << "Starting a new data file." << std::endl;
        return;
    }

    std::error_code error;
    const bool file_exists = std::filesystem::exists(viriformat_output_path, error);
    if (error) {
        throw std::runtime_error("Could not inspect output file: " + error.message());
    }

    std::uintmax_t valid_file_size = 0;
    bool removed_incomplete_game = false;

    if (file_exists) {
        std::ifstream input(viriformat_output_path, std::ios::binary);
        if (!input) {
            throw std::runtime_error("Could not read output file: " + viriformat_output_path);
        }

        while (true) {
            char initial_record[initial_record_size];
            input.read(initial_record, initial_record_size);
            if (input.gcount() == 0) {
                break;
            }
            if (input.gcount() != initial_record_size) {
                removed_incomplete_game = true;
                break;
            }

            int positions_in_game = 0;
            bool game_complete = false;
            while (true) {
                uint8_t move_record[move_record_size];
                input.read(reinterpret_cast<char*>(move_record), move_record_size);
                if (input.gcount() != move_record_size) {
                    removed_incomplete_game = true;
                    break;
                }

                const uint32_t record = static_cast<uint32_t>(move_record[0])
                    | (static_cast<uint32_t>(move_record[1]) << 8)
                    | (static_cast<uint32_t>(move_record[2]) << 16)
                    | (static_cast<uint32_t>(move_record[3]) << 24);

                if (record == 0) {
                    positions_written += positions_in_game;
                    valid_file_size = static_cast<std::uintmax_t>(input.tellg());
                    game_complete = true;
                    break;
                }
                ++positions_in_game;
            }

            if (!game_complete) {
                break;
            }
        }
        input.close();

        const std::uintmax_t existing_file_size = std::filesystem::file_size(viriformat_output_path, error);
        if (error) {
            throw std::runtime_error("Could not get output file size: " + error.message());
        }
        if (valid_file_size < existing_file_size) {
            std::filesystem::resize_file(viriformat_output_path, valid_file_size, error);
            if (error) {
                throw std::runtime_error("Could not remove incomplete trailing game: " + error.message());
            }
            removed_incomplete_game = true;
        }
    }

    out.open(viriformat_output_path, std::ios::binary | std::ios::app);
    if (!out) {
        throw std::runtime_error("Could not open output file for appending: " + viriformat_output_path);
    }

    std::cout << "Resuming with " << positions_written << " complete positions." << std::endl;
    if (removed_incomplete_game) {
        std::cout << "Removed an incomplete trailing game from the previous run." << std::endl;
    }
}

void print_progress(bool force = false) {
    static auto last_update = std::chrono::steady_clock::time_point::min();
    const auto now = std::chrono::steady_clock::now();
    if (!force && now - last_update < std::chrono::milliseconds(250)) {
        return;
    }
    last_update = now;

    const int completed = std::min(positions_written, num_positions);
    const int filled = completed * progress_bar_width / num_positions;
    const int percentage = completed * 100 / num_positions;

    std::cout << "\r[";
    for (int i = 0; i < progress_bar_width; ++i) {
        std::cout << (i < filled ? '#' : ' ');
    }
    std::cout << "] " << percentage << "% ("
              << positions_written << '/' << num_positions << ')' << std::flush;
}

// Need custom function to write because we need to guarentee everything is little endian
void write_u8(uint8_t value) {
    out.write(reinterpret_cast<const char*>(&value), sizeof(uint8_t));
}

void write_s16_le(int16_t value) {
    uint8_t bytes[2] = {
        static_cast<uint8_t>(value & 0xFF),
        static_cast<uint8_t>((value >> 8) & 0xFF)
    };
    out.write(reinterpret_cast<const char*>(bytes), sizeof(bytes));
}


void write_u16_le(uint16_t value) {
    uint8_t bytes[2] = {
        static_cast<uint8_t>(value & 0xFF),
        static_cast<uint8_t>((value >> 8) & 0xFF)
    };
    out.write(reinterpret_cast<const char*>(bytes), sizeof(bytes));
}

void write_u64_le(uint64_t value) {
    uint8_t bytes[8] = {
        static_cast<uint8_t>(value & 0xFF),
        static_cast<uint8_t>((value >> 8) & 0xFF),
        static_cast<uint8_t>((value >> 16) & 0xFF),
        static_cast<uint8_t>((value >> 24) & 0xFF),
        static_cast<uint8_t>((value >> 32) & 0xFF),
        static_cast<uint8_t>((value >> 40) & 0xFF),
        static_cast<uint8_t>((value >> 48) & 0xFF),
        static_cast<uint8_t>((value >> 56) & 0xFF)
    };
    out.write(reinterpret_cast<const char*>(bytes), sizeof(bytes));
}

void write_initial_position(Position& pos, int16_t score, uint8_t game_res) {
    // Occupancy
    Bitboard pieces = pos.pieces();
    write_u64_le(pieces);

    // Piece nibbles
    uint8_t to_write = 0;
    int pieces_written = 0;
    bool flush_next = false;
    Color stm = pos.side_to_move();
    while (pieces) {
        Square sq = pop_lsb(pieces);
        Piece p = pos.board_mailbox[sq];
        PieceType pt = piecetype_of(p);
        Color us = color_of(p);
        uint8_t piece_encoded = pt - pawn;
        if (us == black) {
            piece_encoded |= 1 << 3;
        }
        if (pt == rook && (us == white && ((sq == a1 && (pos.castle_rights & WQ) != 0) || (sq == h1 && (pos.castle_rights & WK) != 0))
            || us == black && ((sq == a8 && (pos.castle_rights & BQ) != 0) || (sq == h8 && (pos.castle_rights & BK) != 0))))
            piece_encoded += 3;

        if (!flush_next){
            to_write |= piece_encoded;
            flush_next = true;
        }
        else {
            to_write |= piece_encoded << 4;
            write_u8(to_write);
            to_write = 0;
            pieces_written += 2;
            flush_next = false;
        }
    }
    if (to_write != 0) {
        write_u8(to_write);
        pieces_written += 2;
    }
    while (pieces_written < 32) {
        write_u8(0);
        pieces_written += 2;
    }

    // Side to move and en passant
    uint8_t stm_ep = stm == white ? 0 : 1 << 7;
    if (pos.en_passant_file == -1) {
        stm_ep |= 64;
    }
    else {
        stm_ep |= make_square((stm == white ? 6 : 3), pos.en_passant_file);
    }
    write_u8(stm_ep);

    // Halfmove clock
    write_u8(pos.halfmove_clock);

    // Fullmove clock
    write_u16_le(pos.fullmove_clock);

    // Score
    write_s16_le(score);

    // Game result
    write_u8(game_res);

    write_u8(0); // extra byte
}

void write_move(Position& pos, Move m, int16_t score) {
    uint16_t from = m.get_from();
    uint16_t to = m.get_to();

    // Castling encoded as capturing rook
    if (m.type_of() == castle) {
        if (to == g1) {
            to = h1;
        }
        else if (to == c1) {
            to = a1;
        }
        else if (to == g8) {
            to = h8;
        }
        else if (to == c8) {
            to = a8;
        }
    }

    uint8_t promotion_piece_type_encoded = 0;
    if (m.type_of() == promotion) {
        promotion_piece_type_encoded = m.get_promotion_piece_type() - knight;
    }

    uint8_t move_type_encoded = 0;
    MoveType mt = m.type_of();
    if (mt == normal)
        move_type_encoded = 0;
    else if (mt == en_passant)
        move_type_encoded = 1;
    else if (mt == castle)
        move_type_encoded = 2;
    else
        move_type_encoded = 3;

    uint16_t move_encoded = from | (to << 6) | (promotion_piece_type_encoded << 12) | (move_type_encoded << 14);

    write_u16_le(move_encoded);
    write_s16_le(score);
}

void write_terminator() {
    uint32_t terminator = 0;
    out.write(reinterpret_cast<const char*>(&terminator), sizeof(uint32_t));
    out.flush();
    if (!out) {
        throw std::runtime_error("Failed while writing output file: " + viriformat_output_path);
    }
}

// This is a very slow way of doing this
std::vector<Move> legal_moves(Position& pos) {
    std::vector<Move> legal;
    legal.reserve(256);
    MoveList moves = generate(pos);
    Color us = pos.side_to_move();
    for (int c = 0; c < moves.count; ++c) {
        pos.make_move(moves.moves[c]);
        if (!pos.is_checked(us)) {
            legal.push_back(moves.moves[c]);
        }
        pos.unmake_move(moves.moves[c]);
    }
    return legal;
}

bool has_legal_moves(Position& pos) {
    MoveList moves = generate(pos);
    Color us = pos.side_to_move();
    for (int c = 0; c < moves.count; ++c) {
        pos.make_move(moves.moves[c]);
        if (!pos.is_checked(us)) {
            pos.unmake_move(moves.moves[c]);
            return true;
        }
        pos.unmake_move(moves.moves[c]);
    }
    return false;
}


int main(int argc, char* argv[]) {
    if (argc > 2) {
        std::cerr << "Usage: DataGeneration.exe [output_file.viri]" << std::endl;
        return 1;
    }

    if (argc == 2) {
        viriformat_output_path = argv[1];
    }

    const std::filesystem::path output_path(viriformat_output_path);
    if (output_path.has_parent_path()) {
        std::error_code error;
        std::filesystem::create_directories(output_path.parent_path(), error);
        if (error) {
            std::cerr << "Could not create output directory: " << error.message() << std::endl;
            return 1;
        }
    }

    prepare_output_file();
    init_magics();
    init_zobrist();

    std::cout << "Writing data to " << viriformat_output_path << std::endl;

    auto start_time = std::chrono::steady_clock::now();

    // UCI engine stores a bunch of stuff (transposition table, etc.) so we have to create it here
    UCIEngine uci = UCIEngine();
    uci.use_soft_nodes = true;
    uci.hash_size = transposition_table_size;
    uci.new_game();

    std::random_device rd;
    std::mt19937 gen(rd());

    print_progress(true);
    while (positions_written < num_positions) {
        uci.reset_position();
        uci.stop_search.store(false);

        std::vector<Move> moves;
        std::vector<int16_t> scores;
        moves.reserve(256);
        scores.reserve(256);

        uci.pos.load_fen(start_fen);
        
        bool skip_position = false;
        // Play random plies
        for (int i = 0; i < num_random_plies; ++i) {
            std::vector<Move> legal = legal_moves(uci.pos);
            if (legal.size() == 0) {
                skip_position = true;
                break;
            }

            std::uniform_int_distribution<int> distrib(0, static_cast<int>(legal.size()) - 1);
            // This is a permanent game move, so it must not consume an entry
            // in Position's fixed-size search undo stack.
            uci.pos.make_move(legal[distrib(gen)], true);
        }
        if (skip_position) {
            skip_position = false;
            continue;
        }
        int result;
		Position initial_pos = uci.pos;
        while (true) {
            if (uci.pos.threefold_or_50_move() || uci.pos.insufficient_material()) {
                result = 1;
                break;
            }
            if (!has_legal_moves(uci.pos)) {
                if (uci.pos.is_checked(uci.pos.side_to_move())) {
                    result = uci.pos.side_to_move() == white ? 0 : 2;
                }
                else {
                    result = 1;
                }
                break;
            }
            std::pair<Move, int16_t> move_and_score = find_best_move_and_score(uci.pos, 999999, uci, 9999, soft_nodes_per_position, false);
            int16_t relative_score = uci.pos.white_to_move ? move_and_score.second : -move_and_score.second;
            moves.push_back(move_and_score.first);
            scores.push_back(relative_score);

            // As above, played game moves do not belong on the search stack.
            uci.pos.make_move(move_and_score.first, true);
            ++positions_written;
            print_progress();
        }

        write_initial_position(initial_pos, 0, result); // Initial score is ignored
        for (size_t i = 0; i < moves.size(); ++i) {
            write_move(uci.pos, moves[i], scores[i]);
        }

        write_terminator();
    }
    print_progress(true);
    std::cout << std::endl;

    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::minutes>(end_time - start_time).count();

    std::cout << "Finished generating data in " << duration << " minutes." << std::endl;
}
