#include "magics.h"

std::array<std::array<Bitboard, 512>, square_num> bishop_attacks;
std::array<std::array<Bitboard, 4096>, square_num> rook_attacks;

std::array<Magic, square_num> bishop_magics;
std::array<Magic, square_num> rook_magics;

Bitboard magic_index(Bitboard blockers, Bitboard magic, int shift_amt) {
    return Bitboard((blockers * magic) >> (shift_amt));
}

template<PieceType T>
auto try_make_table(std::array<Bitboard, T == bishop ? 1 << 9 : 1 << 12>& table,
    std::array<uint32_t, T == bishop ? 1 << 9 : 1 << 12>& epoch,
    int current_epoch,
    Magic magic_entry, Square sq, std::vector<Bitboard>& blocker_subsets, std::vector<Bitboard>& attack_masks) {
    for (size_t i = 0; i < blocker_subsets.size(); ++i) {
        int magic_idx = magic_index(blocker_subsets[i], magic_entry.magic, magic_entry.shift_amt);
        Bitboard attacks = attack_masks[i];
        if (epoch[magic_idx] != current_epoch) {
            table[magic_idx] = attacks;
            epoch[magic_idx] = current_epoch;
        }
        else if (table[magic_idx] != attacks) {
            return false;
        }
    }
    return true;
}


template <PieceType T>
std::array<Magic, square_num> find_magics() {
    std::array<Magic, square_num> magics{};
    // Initialize RNG
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<Bitboard> dis;
    static std::array<Bitboard, T == bishop ? 1 << 9 : 1 << 12> table{};
    static std::array<uint32_t, T == bishop ? 1 << 9 : 1 << 12> epoch{};
    int current_epoch = 0;
    Magic magic_entry{ 0,0,0 };

    for (int i = 0; i < square_num; ++i) {
        Square sq = Square(i);
        Bitboard mask = sliding_attacks(Square(i), T, 0);
        Bitboard edge_mask = ((file_a_bb | file_h_bb) & ~file_bb_of(sq)) | ((rank_1_bb | rank_8_bb) & ~rank_bb_of(sq));
        mask &= ~edge_mask;

        std::vector<Bitboard> blocker_subsets;
        std::vector<Bitboard> attack_masks;

        blocker_subsets.reserve(Bitboard(1) << popcount(mask));
        attack_masks.reserve(Bitboard(1) << popcount(mask));

        Bitboard blockers = 0;
        int j = 0;
        do {
            blocker_subsets.push_back(blockers);
            attack_masks.push_back(sliding_attacks(sq, T, blockers));
            blockers = (blockers - mask) & mask;
        } while (blockers);

        bool found_magic = false;
        do {
            magic_entry = Magic{mask, dis(gen) & dis(gen) & dis(gen), 64 - popcount(mask)};
            found_magic = try_make_table<T>(table, epoch, current_epoch, magic_entry, sq, blocker_subsets, attack_masks);
            ++current_epoch;
        } while (!found_magic);

        magics[i] = magic_entry;
        if constexpr (T == bishop) {
            bishop_attacks[i] = table;
        }
        else {
            rook_attacks[i] = table;
        }
    }
    return magics;
}

void init_magics() {
    bishop_magics = find_magics<bishop>();
    rook_magics = find_magics<rook>();
}

Bitboard get_bishop_attacks(Square sq, Bitboard blockers) {
    Magic magic_entry = bishop_magics[sq];
    Bitboard magic_idx = magic_index(blockers & magic_entry.mask, magic_entry.magic, magic_entry.shift_amt);
    return bishop_attacks[sq][magic_idx];
}

Bitboard get_rook_attacks(Square sq, Bitboard blockers) {
    Magic magic_entry = rook_magics[sq];
    int magic_idx = magic_index(blockers & magic_entry.mask, magic_entry.magic, magic_entry.shift_amt);
    return rook_attacks[sq][magic_idx];
}