// Lightweight script for generating a bunch of NNUE data

#include <iostream>
#include <fstream>
#include "Gishath/position.h"
#include "Gishath/bitboard.h"

// Settings
constexpr int softnodes_per_position = 5000;
constexpr int max_positions = 5'000'000;
constexpr bool prune_bad_positions = true;
const std::string viriformat_output_path = "./viriformat_data.viri";

std::ofstream out(viriformat_output_path, std::ios::binary);
int positions_written = 0;

// Need custom function to write because we need to guarentee everything is little endian
void write_u8(uint8_t value) {
    out.write(reinterpret_cast<const char*>(&value), sizeof(uint8_t));
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
    write_u16_le(score);

    // Game result
    write_u8(game_res);

    write_u8(0); // extra byte
}

void write_move(Position& pos, Move m, uint16_t score) {
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
    write_u16_le(score);
}

void write_terminator() {
    uint32_t terminator = 0;
    out.write(reinterpret_cast<const char*>(&terminator), sizeof(uint32_t));
}

int main() {
    std::cout << "Writing data to " << viriformat_output_path << std::endl;
}