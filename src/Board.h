#pragma once
#include <array>
#include <chrono>
#include "Piece.h"

constexpr int BW = 10;           // board width in cells
constexpr int BH = 20;           // board height in cells
constexpr int STONE_COLOR = 8;   // color id used for stone cells

struct Cell {
    int  color = 0;      // 0 = empty, 1~7 = piece type (PieceType + 1), 8 = stone
    bool bomb = false;  // true if this cell is a bomb
    int  armor = 0;      // 0 = normal, 2 = hard block, 1 = cracked hard block
    std::chrono::steady_clock::time_point lockedAt{};   // when this cell was placed (invisible mode)
};

struct ClearResult {
    int completed = 0;   // rows cleared because they were filled
    int removed = 0;   // all cleared rows (filled rows + rows hit by a blast)
    int bombs = 0;   // number of bombs that exploded
    int chains = 0;   // bombs that exploded because of another blast
};

class Board {
public:
    // true if the piece overlaps a wall, the floor or a placed cell
    bool collides(const Piece& p) const;

    // put the piece into the board. 'now' is remembered for every cell
    void lock(const Piece& p, std::chrono::steady_clock::time_point now);

    // clear filled rows and rows hit by bomb blasts, then drop what is above.
    // chain = true: a bomb inside a blasted row explodes too
    ClearResult clearLines(bool chain);

    // fill the bottom n rows with stone
    void fillStoneRows(int n);

    // push everything up by one row and add a stone row at the bottom.
    // returns false if the top row was already occupied (stack overflow)
    bool pushStoneRow();

    const Cell& at(int row, int col) const { return grid_[row][col]; }

private:
    std::array<std::array<Cell, BW>, BH> grid_{};
};