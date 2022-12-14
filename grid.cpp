#include <iostream>
#include <vector>
#include <stddef.h>
#include <bitset>
#include <x86intrin.h>

const int L = 4;

enum normalizeMode {
    least = 0,
    greatest_flip = 1,
};

enum binMode {
    pext = 0,
    ctz = 1,
};

const enum binMode binMode = binMode::ctz;
const enum normalizeMode normalizeMode = normalizeMode::greatest_flip;

int bin(uint64_t cells) {
    if(binMode == binMode::ctz) {
        return __builtin_ctzll(cells);
    } else {
        return _pext_u64(cells, 0x10307);
    }
}

constexpr int bins() {
    if(binMode == binMode::ctz) {
        if(normalizeMode == normalizeMode::least) {
            return L;
        } else if(normalizeMode == normalizeMode::greatest_flip) {
            return L - 1;
        }
    } else {
        return 64;
    }
}

// format to LxL grid
constexpr uint64_t comboMask() {
    uint64_t mask = 0;
    for(int i = 0; i < L; i++) {
        uint64_t row = (1 << L) - 1;
        mask |= (row << (8 * i));
    }
    return mask;
}

struct board {
    static std::vector<uint64_t> boards[L*L-2][bins()];
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
        int binIndex = bin(cells);
        std::vector<uint64_t>& prevBoards = boards[n-2][binIndex];
        for (uint64_t prevBoard : prevBoards) {
            if (prevBoard == cells) {
                return;
            }
        }
        if (comb(n)) {
            prevBoards.push_back(cells);
        }
        
        return;
    }

    bool comb(int n) {
        flip();
        for (std::vector<uint64_t> prevBoards : boards[n-2]) {
            for (uint64_t prevBoard : prevBoards) {
                if (prevBoard == cells) {
                    rotate();
                    rotate();
                    translate();
                    if (prevBoard == cells) {
                        flip();
                        return false;
                    }
                    rotate();
                    rotate();
                    translate();
                }
            }
        }
        flip();
        return true;
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

    uint64_t flipBits() {
        uint64_t result = 0;
        for(int i = 0; i < 64; i++) {
            result |= ((cells >> i) & 0x1) << (63 - i);
        }

        return result;
    }

    void flip() {
        uint64_t result = 0;
        uint64_t masks[8] = {0xff00000000000000, 0x00ff000000000000, 0x0000ff0000000000, 0x000000ff00000000, 
                             0x00000000ff000000, 0x0000000000ff0000, 0x000000000000ff00, 0x00000000000000ff};
        for(int i=0; i<8; i++) {
            uint64_t selected = _pext_u64(cells, masks[i]);
            result |= selected << (i * 8);
        }
        cells = result;
        transform();
        return;

    }

    void transform() {
        translate();

        if(normalizeMode == normalizeMode::least) {
            uint64_t bestOrient = cells;
            for (int i = 0; i < 3; i++) {
                rotate();
                translate();
                if (cells < bestOrient) {
                    bestOrient = cells;
                }
            }
            cells = bestOrient;
        } else {
            uint64_t bestOrient = flipBits();

            for (int i = 0; i < 3; i++) {
                rotate();
                translate();
                if (flipBits() > bestOrient) {
                    bestOrient = flipBits();
                }
            }

            cells = bestOrient;
            cells = flipBits();
        }
    }
};

std::vector<uint64_t> board::boards[L*L-2][bins()] = {};

uint64_t nextComboPossible(uint64_t combo) {
    // find index of first 0 (from msb) in cells
    int leading_ones = __builtin_clzll(~combo);
    // if top of number is ones, reset
    if (leading_ones != 0) {
        // get mask of leading ones
        uint64_t leading_ones_mask = (int64_t)0x8000000000000000 >> (leading_ones - 1);
        // remove leading ones
        uint64_t cell_advance = combo & ~leading_ones_mask;

        if (cell_advance == 0) {
            return 0;
        }
        // count leading zeros on cell_advance
        int advance_leading_zeros = __builtin_clzll(cell_advance);
        // create mask to advance one
        uint64_t advance_mask = (int64_t)0x8000000000000000 >> (leading_ones + 1);
        
        combo = cell_advance ^ (advance_mask >> (advance_leading_zeros - leading_ones - 1));
    } else {
        // advance first 1
        int first_1_index = 63 - __builtin_clzll(combo);
        combo ^= ((uint64_t)3 << first_1_index);
    }
    return combo;
}

int main() {
    int counts[L*L + 1] = {};
    counts[0] = 0;
    counts[1] = 1;
    counts[L*L] = 1;

    for (int i=2; i<L*L; i++) {
        // create initial board with i cells set
        uint64_t combo = (1 << i) - 1;
        combo <<= 64 - L*L;
        while(combo) {
            board x(combo);
            x.transform();
            x.compare(i);
            combo = nextComboPossible(combo);
        }

        std::cout <<  "======================" << std::endl;
        int count = 0;
        for (int j=0; j<bins(); j++) {
            auto& boards = board::boards[i - 2][j];
            int binCount = 0;
            for (uint64_t cells : boards) {
                if (i == 2) {
                    std::cout << "bins: " << j << std::endl;
                    std::cout << std::bitset<64>(cells) << std::endl;
                    board::fromCells(cells).print();
                    std::cout << std::endl;
                }
                count++;
            }
            std::cout << "bin " << j << ": " << binCount << std::endl;
        }
        std::cout << "n = " << i << ", count = " << count << std::endl;
        counts[i] = count;
    }

    std::cout <<  "======================" << std::endl << "L = " << L << std::endl;
    int sum = 0;
    for(int i = 0; i <= L*L; i++) {
        std::cout << "n = " << i << ",\tcount = " << counts[i] << std::endl;
        sum += counts[i];
    }
    std::cout << "sum_count = " << sum << std::endl;

    return 0;
}