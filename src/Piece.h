#pragma once
#include <array>

constexpr int N = 4;                                   // every piece fits in a 4x4 box
using Shape = std::array<std::array<int, N>, N>;       // 0 empty, 1 normal, 2 bomb, 3 hard block

enum class PieceType { I, O, T, S, Z, J, L };

// A position on the board. operator+ is overloaded so positions can be added.
struct Point {
    int x = 0;
    int y = 0;

    Point operator+(const Point& other) const {
        return Point{ x + other.x, y + other.y };
    }
};

struct Piece {
    PieceType type = PieceType::I;
    Shape cells{};         // current shape (changes when rotated)
    Shape spawnCells{};    // shape when the piece appeared (saved when held)
    int x = 3;             // top-left cell of the 4x4 box (board coordinates)
    int y = 0;
};

Shape baseShape(PieceType t);                          // starting shape
Shape rotateCW(const Shape& s, PieceType t);           // rotate clockwise