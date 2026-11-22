#include "Board.h"
#include <vector>

// A filled row that contains cracked hard blocks (armor 1):
//   true  = it waits and breaks together with the next row clear
//   false = it breaks at the next piece placement by itself
static const bool CRACKED_ROW_WAITS_FOR_CLEAR = true;

bool Board::collides(const Piece& p) const {
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            if (!p.cells[i][j]) continue;

            int r = p.y + i;
            int c = p.x + j;

            if (c < 0 || c >= BW) return true;                    // side walls
            if (r >= BH) return true;                             // floor
            if (r >= 0 && grid_[r][c].color != 0) return true;    // placed cells
        }
    }
    return false;
}

void Board::lock(const Piece& p, std::chrono::steady_clock::time_point now) {
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            int v = p.cells[i][j];
            if (!v) continue;

            int r = p.y + i;
            int c = p.x + j;
            if (r < 0 || r >= BH || c < 0 || c >= BW) continue;

            grid_[r][c].color = static_cast<int>(p.type) + 1;
            grid_[r][c].bomb = (v == 2);
            grid_[r][c].armor = (v == 3) ? 2 : 0;
            grid_[r][c].lockedAt = now;
        }
    }
}

ClearResult Board::clearLines(bool chain) {
    auto rowEmpty = [&](int r) {
        for (int c = 0; c < BW; ++c) {
            if (grid_[r][c].color != 0) return false;
        }
        return true;
        };
    auto rowHasBomb = [&](int r) {
        for (int c = 0; c < BW; ++c) {
            if (grid_[r][c].bomb) return true;
        }
        return false;
        };

    ClearResult res;
    std::array<bool, BH> completed{};   // rows cleared because they are filled
    std::array<bool, BH> cracking{};    // filled rows with fresh hard blocks: they only crack this time
    std::array<bool, BH> waiting{};     // filled rows with cracked hard blocks, waiting for another clear
    std::array<bool, BH> blast{};       // rows hit by a bomb blast
    std::array<bool, BH> exploded{};    // rows whose bomb already exploded
    std::vector<int> pending;           // rows whose bomb is about to explode

    // 1) sort the filled rows
    for (int r = 0; r < BH; ++r) {
        bool full = true;
        bool hasStone = false;
        for (int c = 0; c < BW; ++c) {
            if (grid_[r][c].color == 0) full = false;
            if (grid_[r][c].color == STONE_COLOR) hasStone = true;
        }

        // a row with stone is never cleared by filling it
        if (!full || hasStone) continue;

        bool fresh = false;
        bool cracked = false;
        for (int c = 0; c < BW; ++c) {
            if (grid_[r][c].armor == 2) fresh = true;
            else if (grid_[r][c].armor == 1) cracked = true;
        }

        if (fresh) {
            cracking[r] = true;                         // hard block: the row is NOT cleared, it only cracks
        }
        else if (cracked && CRACKED_ROW_WAITS_FOR_CLEAR) {
            waiting[r] = true;                          // cracked row: breaks with the next clear
        }
        else {
            completed[r] = true;                        // normal row
            if (rowHasBomb(r)) pending.push_back(r);
        }
    }
    int initialBombs = static_cast<int>(pending.size());

    // 2) bomb blasts: the row above and the row below are blasted too.
    //    with chain on, a bomb in a blasted row explodes as well
    while (!pending.empty()) {
        int r = pending.back();
        pending.pop_back();
        if (exploded[r]) continue;
        exploded[r] = true;
        ++res.bombs;

        for (int nr : {r - 1, r + 1}) {
            if (nr < 0 || nr >= BH) continue;
            if (rowEmpty(nr) || blast[nr]) continue;

            blast[nr] = true;
            if (chain && !completed[nr] && rowHasBomb(nr)) {
                pending.push_back(nr);
            }
        }
    }
    res.chains = res.bombs - initialBombs;

    // 3) if any row is cleared right now, the waiting cracked rows break together with it
    bool anyClear = false;
    for (int r = 0; r < BH; ++r) {
        if (completed[r] || blast[r]) anyClear = true;
    }
    if (anyClear) {
        for (int r = 0; r < BH; ++r) {
            if (waiting[r]) completed[r] = true;
        }
    }

    // 4) fresh hard blocks in filled rows crack (the rows stay as they are, nothing above moves)
    for (int r = 0; r < BH; ++r) {
        if (!cracking[r]) continue;
        for (int c = 0; c < BW; ++c) {
            if (grid_[r][c].armor == 2) grid_[r][c].armor = 1;
        }
    }

    // 5) remove the cleared rows and drop the rows above (bottom to top)
    std::array<bool, BH> removeRow{};
    for (int r = 0; r < BH; ++r) {
        removeRow[r] = completed[r] || blast[r];
        if (removeRow[r]) ++res.removed;
        if (completed[r]) ++res.completed;
    }
    if (res.removed == 0) return res;

    int write = BH - 1;
    for (int read = BH - 1; read >= 0; --read) {
        if (removeRow[read]) continue;
        if (write != read) grid_[write] = grid_[read];
        --write;
    }
    for (; write >= 0; --write) {
        grid_[write] = {};
    }
    return res;
}

void Board::fillStoneRows(int n) {
    for (int i = 0; i < n; ++i) {
        int r = BH - 1 - i;
        if (r < 0) break;
        for (int c = 0; c < BW; ++c) {
            grid_[r][c] = Cell{ STONE_COLOR, false, 0 };
        }
    }
}

bool Board::pushStoneRow() {
    bool overflow = false;
    for (int c = 0; c < BW; ++c) {
        if (grid_[0][c].color != 0) overflow = true;
    }

    for (int r = 0; r < BH - 1; ++r) {
        grid_[r] = grid_[r + 1];
    }
    for (int c = 0; c < BW; ++c) {
        grid_[BH - 1][c] = Cell{ STONE_COLOR, false, 0 };
    }
    return !overflow;
}