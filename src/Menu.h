#pragma once
#include "Game.h"
#include "Scene.h"
#include "Screen.h"

enum class MenuResult { None, Start, Quit };

// The screen where you pick a mode and its options before the game starts.
// It is one kind of Scene (inheritance). Each mode shows only its own option rows.
class Menu : public Scene {
public:
    // Scene interface (overridden virtual functions)
    void handle(Key k) override;
    void draw(Screen& s) const override;
    SceneExit exitRequest() const override;

    Rules rules() const;          // the rules chosen in the menu
    void clearRequest();          // forget the last Start / Quit request

private:
    enum class Row { Mode, Rotation, Types, Armor, Difficulty, Chain, Fade, Start };

    MenuResult process(Key k);    // handles one key and says what happened
    int rowCount() const;
    Row rowAt(int index) const;

    MenuResult result_ = MenuResult::None;

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