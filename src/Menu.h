#pragma once
#include "Game.h"
#include "Screen.h"

enum class MenuResult { None, Start, Quit };

// The screen where you pick a mode and its options before the game starts.
// Each mode shows only its own option rows.
class Menu {
public:
    MenuResult handle(Key k);
    void draw(Screen& s) const;
    Rules rules() const;

private:
    enum class Row { Mode, Rotation, Types, Armor, Difficulty, Chain, Fade, Start };

    int rowCount() const;
    Row rowAt(int index) const;

    int       cursor_ = 0;
    GameMode  mode_ = GameMode::Standard;

    // Standard mode
    bool      rotation_ = true;
    int       typeCount_ = 7;
    int       armorIdx_ = 0;            // 0 = off, then the cooldown choices

    // Bomb mode
    BombLevel level_ = BombLevel::Normal;
    bool      chain_ = false;

    // Invisible mode
    FadeLevel fade_ = FadeLevel::Normal;
};