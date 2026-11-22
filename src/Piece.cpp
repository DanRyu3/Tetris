#include "Piece.h"

Shape baseShape(PieceType t) {
    switch (t) {
    case PieceType::I: return { {{0,0,0,0}, {1,1,1,1}, {0,0,0,0}, {0,0,0,0}} };
    case PieceType::O: return { {{0,1,1,0}, {0,1,1,0}, {0,0,0,0}, {0,0,0,0}} };
    case PieceType::T: return { {{0,1,0,0}, {1,1,1,0}, {0,0,0,0}, {0,0,0,0}} };
    case PieceType::S: return { {{0,1,1,0}, {1,1,0,0}, {0,0,0,0}, {0,0,0,0}} };
    case PieceType::Z: return { {{1,1,0,0}, {0,1,1,0}, {0,0,0,0}, {0,0,0,0}} };
    case PieceType::J: return { {{1,0,0,0}, {1,1,1,0}, {0,0,0,0}, {0,0,0,0}} };
    case PieceType::L: return { {{0,0,1,0}, {1,1,1,0}, {0,0,0,0}, {0,0,0,0}} };
    }
    return {};
}

Shape rotateCW(const Shape& s, PieceType t) {
    if (t == PieceType::O) return s;                   // 정사각형은 회전해도 같음

    const int n = (t == PieceType::I) ? 4 : 3;         // 회전 기준 행렬 크기
    Shape r{};
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            r[i][j] = s[n - 1 - j][i];                 // new[i][j] = old[n-1-j][i]
        }
    }
    return r;
}