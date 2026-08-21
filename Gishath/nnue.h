#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

#include "types.h"

class Position;

namespace nnue {

constexpr std::size_t input_size = 768;
constexpr std::size_t hidden_size = 64;
constexpr std::int16_t qa = 255;
constexpr std::int16_t qb = 32;
constexpr int output_scale = 200; // Scales output back up to centipawns

struct alignas(16) Accumulator {
    std::array<std::int16_t, hidden_size> values{};
};

// Loads Bullet's quantised (768 -> hidden_size) x 2 -> 1 network format.
// If this is not called explicitly, the first refresh/evaluation searches the
// GISHATH_NNUE_FILE environment variable, the working directory, and the
// project root or executable's directory for "nnue.bin".
std::string default_network_path();
void load_network(const std::string& path);
bool network_loaded();

void refresh(Position& position);
void add_piece(Position& position, Piece piece, Square square);
void remove_piece(Position& position, Piece piece, Square square);
int evaluate(const Position& position);

} // namespace nnue
