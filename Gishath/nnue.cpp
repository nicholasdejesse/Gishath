#include "nnue.h"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <vector>

#include "position.h"

#if defined(_WIN32)
#define NOMINMAX
#include <Windows.h>
#endif

#if defined(_MSC_VER) && defined(_M_X64)
#include <immintrin.h>
#define GISHATH_NNUE_X86 1
#elif (defined(__i386__) || defined(__x86_64__)) && defined(__SSE2__)
#include <immintrin.h>
#define GISHATH_NNUE_X86 1
#endif

#if defined(__ARM_NEON) || defined(__ARM_NEON__) || defined(_M_ARM64)
#include <arm_neon.h>
#define GISHATH_NNUE_NEON 1
#endif

namespace nnue {
namespace {

struct Network {
    std::array<Accumulator, input_size> feature_weights{};
    Accumulator feature_bias{};
    std::array<std::int16_t, 2 * hidden_size> output_weights{};
    std::int16_t output_bias = 0;
};

Network network;
bool loaded = false;
std::mutex network_mutex;

constexpr std::size_t unpadded_network_size =
    input_size * hidden_size * sizeof(std::int16_t)
    + hidden_size * sizeof(std::int16_t)
    + 2 * hidden_size * sizeof(std::int16_t)
    + sizeof(std::int16_t);
constexpr std::size_t padded_network_size = (unpadded_network_size + 63) & ~std::size_t(63);

void read_value(const std::vector<char>& bytes, std::size_t& offset, std::int16_t& value) {
    if (offset + sizeof(std::int16_t) > bytes.size()) {
        throw std::runtime_error("NNUE file ended before all weights were read");
    }
    const auto low = static_cast<std::uint8_t>(bytes[offset]);
    const auto high = static_cast<std::uint8_t>(bytes[offset + 1]);
    value = static_cast<std::int16_t>(static_cast<std::uint16_t>(low) | (static_cast<std::uint16_t>(high) << 8));
    offset += sizeof(std::int16_t);
}

void read_network_file(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        throw std::runtime_error("Could not open NNUE network: " + path);
    }

    const auto length = file.tellg();
    if (length < 0
        || (static_cast<std::size_t>(length) != unpadded_network_size
            && static_cast<std::size_t>(length) != padded_network_size)) {
        throw std::runtime_error(
            "NNUE network has the wrong size for (768 -> " + std::to_string(hidden_size)
            + ") x 2 -> 1: " + path);
    }

    std::vector<char> bytes(static_cast<std::size_t>(length));
    file.seekg(0);
    file.read(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    if (!file) {
        throw std::runtime_error("Could not read NNUE network: " + path);
    }

    Network next{};
    std::size_t offset = 0;
    for (auto& feature : next.feature_weights) {
        for (auto& weight : feature.values) {
            read_value(bytes, offset, weight);
        }
    }
    for (auto& bias : next.feature_bias.values) {
        read_value(bytes, offset, bias);
    }
    for (auto& weight : next.output_weights) {
        read_value(bytes, offset, weight);
    }
    read_value(bytes, offset, next.output_bias);

    network = next;
    loaded = true;
}

std::optional<std::string> configured_network_path() {
#if defined(_MSC_VER)
    char* value = nullptr;
    std::size_t length = 0;
    if (_dupenv_s(&value, &length, "GISHATH_NNUE_FILE") != 0 || value == nullptr) {
        return std::nullopt;
    }
    const std::string path(value);
    std::free(value);
    return path;
#else
    if (const char* value = std::getenv("GISHATH_NNUE_FILE")) {
        return std::string(value);
    }
    return std::nullopt;
#endif
}

std::optional<std::filesystem::path> executable_directory() {
#if defined(_WIN32)
    std::wstring path(32768, L'\0');
    const DWORD length = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
    if (length == 0 || length == path.size()) {
        return std::nullopt;
    }
    path.resize(length);
    return std::filesystem::path(path).parent_path();
#else
    return std::nullopt;
#endif
}

void ensure_loaded() {
    std::scoped_lock lock(network_mutex);
    if (loaded) {
        return;
    }

    if (const auto configured = configured_network_path()) {
        read_network_file(*configured);
        return;
    }

    if (std::filesystem::is_regular_file("nnue.bin")) {
        read_network_file("nnue.bin");
        return;
    }

    if (const auto directory = executable_directory()) {
        const auto beside_executable = *directory / "nnue.bin";
        if (std::filesystem::is_regular_file(beside_executable)) {
            read_network_file(beside_executable.string());
            return;
        }
    }

    throw std::runtime_error(
        "No NNUE network found. Put nnue.bin in the working directory or beside "
        "Gishath.exe, or set GISHATH_NNUE_FILE to the network path.");
}

constexpr std::size_t feature_index(Color perspective, Piece piece, Square square) {
    const Color piece_color = color_of(piece);
    const std::size_t piece_type = static_cast<std::size_t>(piecetype_of(piece) - pawn);
    const std::size_t color_offset = piece_color == perspective ? 0 : 384;
    const Square oriented_square = perspective == white ? square : relative_square(black, square);
    return color_offset + piece_type * 64 + static_cast<std::size_t>(oriented_square);
}

void add_scalar(Accumulator& accumulator, const Accumulator& weights) {
    for (std::size_t i = 0; i < hidden_size; ++i) {
        accumulator.values[i] = static_cast<std::int16_t>(accumulator.values[i] + weights.values[i]);
    }
}

void subtract_scalar(Accumulator& accumulator, const Accumulator& weights) {
    for (std::size_t i = 0; i < hidden_size; ++i) {
        accumulator.values[i] = static_cast<std::int16_t>(accumulator.values[i] - weights.values[i]);
    }
}

void add(Accumulator& accumulator, const Accumulator& weights) {
#if defined(GISHATH_NNUE_X86)
    std::size_t i = 0;
    for (; i + 8 <= hidden_size; i += 8) {
        const __m128i a = _mm_load_si128(reinterpret_cast<const __m128i*>(accumulator.values.data() + i));
        const __m128i w = _mm_load_si128(reinterpret_cast<const __m128i*>(weights.values.data() + i));
        _mm_store_si128(reinterpret_cast<__m128i*>(accumulator.values.data() + i), _mm_add_epi16(a, w));
    }
    for (; i < hidden_size; ++i) {
        accumulator.values[i] = static_cast<std::int16_t>(accumulator.values[i] + weights.values[i]);
    }
#elif defined(GISHATH_NNUE_NEON)
    std::size_t i = 0;
    for (; i + 8 <= hidden_size; i += 8) {
        const int16x8_t a = vld1q_s16(accumulator.values.data() + i);
        const int16x8_t w = vld1q_s16(weights.values.data() + i);
        vst1q_s16(accumulator.values.data() + i, vaddq_s16(a, w));
    }
    for (; i < hidden_size; ++i) {
        accumulator.values[i] = static_cast<std::int16_t>(accumulator.values[i] + weights.values[i]);
    }
#else
    add_scalar(accumulator, weights);
#endif
}

void subtract(Accumulator& accumulator, const Accumulator& weights) {
#if defined(GISHATH_NNUE_X86)
    std::size_t i = 0;
    for (; i + 8 <= hidden_size; i += 8) {
        const __m128i a = _mm_load_si128(reinterpret_cast<const __m128i*>(accumulator.values.data() + i));
        const __m128i w = _mm_load_si128(reinterpret_cast<const __m128i*>(weights.values.data() + i));
        _mm_store_si128(reinterpret_cast<__m128i*>(accumulator.values.data() + i), _mm_sub_epi16(a, w));
    }
    for (; i < hidden_size; ++i) {
        accumulator.values[i] = static_cast<std::int16_t>(accumulator.values[i] - weights.values[i]);
    }
#elif defined(GISHATH_NNUE_NEON)
    std::size_t i = 0;
    for (; i + 8 <= hidden_size; i += 8) {
        const int16x8_t a = vld1q_s16(accumulator.values.data() + i);
        const int16x8_t w = vld1q_s16(weights.values.data() + i);
        vst1q_s16(accumulator.values.data() + i, vsubq_s16(a, w));
    }
    for (; i < hidden_size; ++i) {
        accumulator.values[i] = static_cast<std::int16_t>(accumulator.values[i] - weights.values[i]);
    }
#else
    subtract_scalar(accumulator, weights);
#endif
}

int dot_scalar(const Accumulator& accumulator, const std::int16_t* weights) {
    int result = 0;
    for (std::size_t i = 0; i < hidden_size; ++i) {
        const int value = std::clamp<int>(accumulator.values[i], 0, qa);
        result += value * value * static_cast<int>(weights[i]);
    }
    return result;
}

#if defined(GISHATH_NNUE_NEON)
int dot_neon(const Accumulator& accumulator, const std::int16_t* weights) {
    int result = 0;
    std::size_t i = 0;
    for (; i + 8 <= hidden_size; i += 8) {
        int16x8_t values = vld1q_s16(accumulator.values.data() + i);
        values = vmaxq_s16(values, vdupq_n_s16(0));
        values = vminq_s16(values, vdupq_n_s16(qa));

        const int16x8_t weight_values = vld1q_s16(weights + i);
        const int32x4_t squares_low = vmull_s16(vget_low_s16(values), vget_low_s16(values));
        const int32x4_t squares_high = vmull_s16(vget_high_s16(values), vget_high_s16(values));
        const int32x4_t weighted_low = vmulq_s32(squares_low, vmovl_s16(vget_low_s16(weight_values)));
        const int32x4_t weighted_high = vmulq_s32(squares_high, vmovl_s16(vget_high_s16(weight_values)));
        const int32x4_t sum = vaddq_s32(weighted_low, weighted_high);
#if defined(__aarch64__) || defined(_M_ARM64)
        result += vaddvq_s32(sum);
#else
        const int32x2_t pair = vadd_s32(vget_low_s32(sum), vget_high_s32(sum));
        result += vget_lane_s32(vpadd_s32(pair, pair), 0);
#endif
    }
    for (; i < hidden_size; ++i) {
        const int value = std::clamp<int>(accumulator.values[i], 0, qa);
        result += value * value * static_cast<int>(weights[i]);
    }
    return result;
}
#endif

#if defined(GISHATH_NNUE_X86)
// SSE2 implementation. Squared clipped values are unsigned 16-bit numbers;
// _mm_madd_epi16 is signed, so add 2^16 * weight where the square's sign bit
// is set to recover the equivalent unsigned product.
int dot_sse2(const Accumulator& accumulator, const std::int16_t* weights) {
    int result = 0;
    const __m128i ones = _mm_set1_epi16(1);
    std::size_t i = 0;
    for (; i + 8 <= hidden_size; i += 8) {
        __m128i values = _mm_load_si128(reinterpret_cast<const __m128i*>(accumulator.values.data() + i));
        values = _mm_max_epi16(values, _mm_setzero_si128());
        values = _mm_min_epi16(values, _mm_set1_epi16(qa));
        const __m128i weight_values = _mm_loadu_si128(reinterpret_cast<const __m128i*>(weights + i));
        const __m128i squares = _mm_mullo_epi16(values, values);
        const __m128i high_bits = _mm_cmpgt_epi16(_mm_setzero_si128(), squares);
        const __m128i correction_weights = _mm_and_si128(high_bits, weight_values);

        __m128i sum = _mm_madd_epi16(squares, weight_values);
        const __m128i correction = _mm_slli_epi32(_mm_madd_epi16(correction_weights, ones), 16);
        sum = _mm_add_epi32(sum, correction);
        sum = _mm_add_epi32(sum, _mm_srli_si128(sum, 8));
        sum = _mm_add_epi32(sum, _mm_srli_si128(sum, 4));
        result += _mm_cvtsi128_si32(sum);
    }
    for (; i < hidden_size; ++i) {
        const int value = std::clamp<int>(accumulator.values[i], 0, qa);
        result += value * value * static_cast<int>(weights[i]);
    }
    return result;
}
#endif

int dot(const Accumulator& accumulator, const std::int16_t* weights) {
#if defined(GISHATH_NNUE_NEON)
    const int result = dot_neon(accumulator, weights);
#elif defined(GISHATH_NNUE_X86)
    const int result = dot_sse2(accumulator, weights);
#else
    return dot_scalar(accumulator, weights);
#endif
#if defined(GISHATH_NNUE_NEON) || defined(GISHATH_NNUE_X86)
#ifndef NDEBUG
    if (result != dot_scalar(accumulator, weights)) {
        throw std::runtime_error("SIMD NNUE result differs from scalar fallback");
    }
#endif
    return result;
#endif
}

} // namespace

void load_network(const std::string& path) {
    std::scoped_lock lock(network_mutex);
    read_network_file(path);
}

bool network_loaded() {
    std::scoped_lock lock(network_mutex);
    return loaded;
}

void refresh(Position& position) {
    ensure_loaded();
    position.nnue_accumulators[white] = network.feature_bias;
    position.nnue_accumulators[black] = network.feature_bias;
    position.nnue_accumulator_valid = true;

    for (Square square = a1; square < square_num; square = Square(square + 1)) {
        const Piece piece = position.board_mailbox[square];
        if (piece != no_piece) {
            add_piece(position, piece, square);
        }
    }
}

void add_piece(Position& position, Piece piece, Square square) {
    if (!position.nnue_accumulator_valid) {
        return;
    }
    add(position.nnue_accumulators[white], network.feature_weights[feature_index(white, piece, square)]);
    add(position.nnue_accumulators[black], network.feature_weights[feature_index(black, piece, square)]);
}

void remove_piece(Position& position, Piece piece, Square square) {
    if (!position.nnue_accumulator_valid) {
        return;
    }
    subtract(position.nnue_accumulators[white], network.feature_weights[feature_index(white, piece, square)]);
    subtract(position.nnue_accumulators[black], network.feature_weights[feature_index(black, piece, square)]);
}

int evaluate(const Position& position) {
    ensure_loaded();
    if (!position.nnue_accumulator_valid) {
        throw std::runtime_error("Attempted NNUE evaluation with an invalid accumulator");
    }

#ifndef NDEBUG
    std::array<Accumulator, color_num> expected{network.feature_bias, network.feature_bias};
    for (Square square = a1; square < square_num; square = Square(square + 1)) {
        const Piece piece = position.board_mailbox[square];
        if (piece != no_piece) {
            add(expected[white], network.feature_weights[feature_index(white, piece, square)]);
            add(expected[black], network.feature_weights[feature_index(black, piece, square)]);
        }
    }
    if (expected[white].values != position.nnue_accumulators[white].values
        || expected[black].values != position.nnue_accumulators[black].values) {
        throw std::runtime_error("Incremental NNUE accumulator is inconsistent with the board");
    }
#endif

    const Color us = position.white_to_move ? white : black;
    const Color them = Color(1 - us);
    int output = dot(position.nnue_accumulators[us], network.output_weights.data());
    output += dot(position.nnue_accumulators[them], network.output_weights.data() + hidden_size);
    output /= qa;
    output += network.output_bias;
    output *= output_scale;
    output /= static_cast<int>(qa) * static_cast<int>(qb);
    return output;
}

} // namespace nnue
