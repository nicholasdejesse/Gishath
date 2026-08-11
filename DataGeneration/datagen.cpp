// Lightweight script for generating a bunch of NNUE data

#include <iostream>
#include <fstream>
#include <random>
#include <vector>
#include "Gishath/position.h"
#include "Gishath/bitboard.h"
#include "Gishath/uci.h"
#include "Gishath/eval.h"
#include <chrono>

// Settings
constexpr int soft_nodes_per_position = 5000;
constexpr int num_positions = 5000; // This is a soft limit, final game will not be truncated
constexpr int num_random_plies = 10; // Number of random plies to play at start of game
constexpr int transposition_table_size = 64; // MB
const std::string viriformat_output_path = "./viriformat_data.viri";

std::ofstream out(viriformat_output_path, std::ios::binary);
int positions_written = 0;

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


int main() {
    std::cout << "Writing data to " << viriformat_output_path << std::endl;

    auto start_time = std::chrono::steady_clock::now();

    // UCI engine stores a bunch of stuff (transposition table, etc.) so we have to create it here
    UCIEngine uci = UCIEngine();
    uci.use_soft_nodes = true;
    uci.hash_size = transposition_table_size;

    std::random_device rd;
    std::mt19937 gen(rd());

    while (positions_written < num_positions) {
        uci.new_game();

        std::vector<Move> moves;
        std::vector<int16_t> scores;
        moves.reserve(256);
        scores.reserve(256);

        Position pos = Position();
        pos.load_fen(start_fen);
        
        bool skip_position = false;
        // Play random plies
        for (int i = 0; i < num_random_plies; ++i) {
            std::vector<Move> legal = legal_moves(pos);
            if (legal.size() == 0) {
                skip_position = true;
                break;
            }

            std::uniform_int_distribution<int> distrib(0, static_cast<int>(legal.size()) - 1);
            pos.make_move(legal[distrib(gen)]);
        }
        if (skip_position) {
            skip_position = false;
            continue;
        }
        Position initial_pos = pos;
        int result;
        while (true) {
            if (pos.threefold_or_50_move() || pos.insufficient_material()) {
                result = 1;
                break;
            }
            if (!has_legal_moves(pos)) {
                if (pos.is_checked(pos.side_to_move())) {
                    result = pos.side_to_move() == white ? 0 : 2;
                }
                else {
                    result = 1;
                }
                break;
            }
            std::pair<Move, int16_t> move_and_score = find_best_move_and_score(pos, 999999, uci, 9999, soft_nodes_per_position, true);
            int16_t relative_score = pos.white_to_move ? move_and_score.second : -move_and_score.second;
            moves.push_back(move_and_score.first);
            scores.push_back(relative_score);

            pos.make_move(move_and_score.first);
            ++positions_written;
        }

        write_initial_position(initial_pos, 0, result); // Initial score is ignored
        for (size_t i = 0; i < moves.size(); ++i) {
            write_move(pos, moves[i], scores[i]);
        }

        write_terminator();
    }

    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::minutes>(end_time - start_time).count();

    std::cout << "Finished generating data in " << duration << " minutes." << std::endl;
}