#include <iostream>
#include <vector>
#include <stddef.h>
#include <bitset>
#include <x86intrin.h>

#define L 5
#define BINS 64
#define BINMASK 0x10307

constexpr uint64_t comboMask() {
    uint64_t mask = 0;
    for(int i = 0; i < L; i++) {
        uint64_t row = (1 << L) - 1;
        mask |= (row << (8 * i));
    }
    return mask;
}

struct board {
    static std::vector<uint64_t> boards[L*L-2][BINS];
    uint64_t cells;

    board(): cells(0) {};

    board(uint64_t combo) {
        // move combo bits to lsb side
        combo >>= (64 - L*L);
        cells = _pdep_u64(combo, comboMask());
    }

    static board fromCells(uint64_t cells) {
        board b{};
        b.cells = cells;
        return b;
    }

    void compare(int n) {
        uint64_t bin = _pext_u64(cells, BINMASK);
        std::vector<uint64_t>& prevBoards = boards[n-2][bin];
        for (uint64_t prevBoard : prevBoards) {
            if (prevBoard == cells) {
                return;
            }
        }
        prevBoards.push_back(cells);
        return;
    }

    void print() {
        uint64_t temp = cells;
        for (int i=0; i<64; i++) {
            std::cout << ((temp & 1) ? 'X' : '.') << " ";
            temp >>= 1;
            if (i % 8 == 7) {
                std::cout << std::endl;
            }
        }
        return;
    }

    void rotate() {
        uint64_t result = 0;
        uint64_t masks[8] = {0x8080808080808080, 0x4040404040404040, 0x2020202020202020, 0x1010101010101010, 
                             0x0808080808080808, 0x0404040404040404, 0x0202020202020202, 0x0101010101010101};
        for(int i=0; i<8; i++) {
            uint64_t selected = _pext_u64(cells, masks[i]);
            result |= selected << (i * 8);
        }
        cells = result;
        return;
    }

    void translate() {
        while (!(cells & 0xff)) {
            cells >>= 8;
        }
        while (!(cells & 0x101010101010101)) {
            cells >>= 1;
        }
        return;
    }

    void transform() {
        translate();

        uint64_t least = cells;
        for (int i=0; i<3; i++) {
            rotate();
            translate();
            if(cells < least) {
                least = cells;
            }
        }
        cells = least;
        return;
    }
};

std::vector<uint64_t> board::boards[L*L-2][BINS] = {};

uint64_t nextComboPossible(uint64_t cell) {
    // find index of first 0 (from msb) in cells
    int leading_ones = __builtin_clzll(~cell);
    // if top of number is ones, reset
    if(leading_ones != 0) {
        // get mask of leading ones
        uint64_t leading_ones_mask = (int64_t)0x8000000000000000 >> (leading_ones - 1);
        // remove leading ones
        uint64_t cell_advance = cell & ~leading_ones_mask;

        if(cell_advance == 0) {
            return 0;
        }
        // count leading zeros on cell_advance
        int advance_leading_zeros = __builtin_clzll(cell_advance);
        // create mask to advance one
        uint64_t advance_mask = (int64_t)0x8000000000000000 >> (leading_ones + 1);
        
        cell = cell_advance ^ (advance_mask >> (advance_leading_zeros - leading_ones - 1));
    } else {
        // advance first 1
        int first_1_index = 63 - __builtin_clzll(cell);
        cell ^= ((uint64_t)3 << first_1_index);
    }

    return cell;
}

int main() {
    int counts[L*L + 1] = {};
    counts[0] = 0;
    counts[1] = 1;
    counts[L*L] = 1;

    for (int i=2; i < L*L; i++) {
        // create initial board with i cells set
        uint64_t combo = (1 << i) - 1;
        combo <<= 64 - L*L;
        while(combo) {
            board x(combo);
            x.transform();
            x.compare(i);
            combo = nextComboPossible(combo);
        }

        std::cout <<  "======================" << std::endl << "n = " << i << std::endl << std::endl;
        int count = 0;
        for (int j=0; j<BINS; j++) {
            auto& boards = board::boards[i - 2][j];
            for(uint64_t cells : boards) {
                //board::fromCells(cells).print();
                //std::cout << std::endl;
                count++;
            }
        }
        std::cout << "n = " << i << ", count = " << count << std::endl << std::endl;

        counts[i] = count;
    }

    std::cout <<  "====================" << std::endl << "L = " << L << std::endl;
    int sum = 0;
    for(int i = 0; i <= L*L; i++) {
        std::cout << "n = " << i << ",\tcount = " << counts[i] << std::endl;
        sum += counts[i];
    }
    std::cout << "sum_count = " << sum << std::endl;

    return 0;
}