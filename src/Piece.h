#pragma once
#include <array>

constexpr int N = 4;                                   // 모든 블록을 4x4 안에 담음
using Shape = std::array<std::array<int, N>, N>;       // 0 빈칸, 1 일반 블록, 2 폭탄(나중에 사용)

enum class PieceType { I, O, T, S, Z, J, L };

struct Piece {
    PieceType type = PieceType::I;
    Shape cells{};         // 현재 모양 (회전하면 바뀜)
    Shape spawnCells{};    // 생성될 때의 모양 (홀드할 때 이 모양으로 저장함)
    int x = 3;             // 4x4 상자의 왼쪽 위 칸 (보드 기준)
    int y = 0;
};

Shape baseShape(PieceType t);                          // 처음 모양
Shape rotateCW(const Shape& s, PieceType t);           // 시계 방향 회전