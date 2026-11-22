#include "Menu.h"
#include <algorithm>
#include <cstdio>

// hard block cooldown choices (0 = off)
static const int ARMOR_VALUES[] = { 0, 12, 8, 5 };
static const int ARMOR_COUNT = 4;

int Menu::rowCount() const {
    switch (mode_) {
    case GameMode::Standard:  return 5;   // mode, rotation, types, hard block, start
    case GameMode::Bomb:      return 4;   // mode, difficulty, chain, start
    case GameMode::Invisible: return 3;   // mode, fade time, start
    }
    return 1;
}

Menu::Row Menu::rowAt(int index) const {
    static const Row standardRows[] = { Row::Mode, Row::Rotation, Row::Types, Row::Armor, Row::Start };
    static const Row bombRows[] = { Row::Mode, Row::Difficulty, Row::Chain, Row::Start };
    static const Row invisibleRows[] = { Row::Mode, Row::Fade, Row::Start };

    switch (mode_) {
    case GameMode::Standard:  return standardRows[index];
    case GameMode::Bomb:      return bombRows[index];
    case GameMode::Invisible: return invisibleRows[index];
    }
    return Row::Start;
}

Rules Menu::rules() const {
    Rules r;
    r.mode = mode_;
    switch (mode_) {
    case GameMode::Standard:
        r.allowRotation = rotation_;
        r.typeCount = typeCount_;
        r.armorEvery = ARMOR_VALUES[armorIdx_];
        break;
    case GameMode::Bomb:
        r.bombLevel = level_;
        r.bombChain = chain_;
        break;
    case GameMode::Invisible:
        r.fadeLevel = fade_;
        break;
    }
    return r;
}

MenuResult Menu::handle(Key k) {
    switch (k) {
    case Key::Up:
        cursor_ = (cursor_ + rowCount() - 1) % rowCount();
        break;
    case Key::Down:
        cursor_ = (cursor_ + 1) % rowCount();
        break;
    case Key::Left:
    case Key::Right: {
        int dir = (k == Key::Right) ? 1 : -1;
        switch (rowAt(cursor_)) {
        case Row::Mode: {
            int m = (static_cast<int>(mode_) + dir + 3) % 3;
            mode_ = static_cast<GameMode>(m);
            break;
        }
        case Row::Rotation:
            rotation_ = !rotation_;
            break;
        case Row::Types:
            typeCount_ = std::clamp(typeCount_ + dir, 1, 7);
            break;
        case Row::Armor:
            armorIdx_ = (armorIdx_ + dir + ARMOR_COUNT) % ARMOR_COUNT;
            break;
        case Row::Difficulty: {
            int d = (static_cast<int>(level_) + dir + 3) % 3;
            level_ = static_cast<BombLevel>(d);
            break;
        }
        case Row::Chain:
            chain_ = !chain_;
            break;
        case Row::Fade: {
            int d = (static_cast<int>(fade_) + dir + 3) % 3;
            fade_ = static_cast<FadeLevel>(d);
            break;
        }
        case Row::Start:
            break;
        }
        break;
    }
    case Key::Enter:
    case Key::Space:
        return MenuResult::Start;
    case Key::Quit:
        return MenuResult::Quit;
    default:
        break;
    }
    return MenuResult::None;
}

void Menu::draw(Screen& s) const {
    s.clear();

    // colorful title
    const char* title = "TETRIS";
    const uint32_t titleColors[] = { 0x00E5FF, 0xFFD600, 0xAA00FF, 0x00E676, 0xFF1744, 0xFF9100 };
    for (int i = 0; i < 6; ++i) {
        s.put(1, 4 + i * 2, static_cast<char32_t>(title[i]), titleColors[i], 0x000000);
    }

    s.text(3, 4, "MODE SELECT", 0x999999);

    // option rows
    int y = 5;
    for (int i = 0; i < rowCount(); ++i) {
        Row row = rowAt(i);
        bool selected = (cursor_ == i);
        uint32_t color = selected ? 0xFFD600 : 0xFFFFFF;

        if (row == Row::Start) ++y;                 // blank line before START
        if (selected) s.text(y, 2, ">", color);

        char buf[24];
        const char* levelNames[] = { "<  EASY    >", "<  NORMAL  >", "<  HARD    >" };
        switch (row) {
        case Row::Mode: {
            const char* modeNames[] = { "<  STANDARD  >", "<  BOMB      >", "<  INVISIBLE >" };
            s.text(y, 4, "MODE", color);
            s.text(y, 20, modeNames[static_cast<int>(mode_)], color);
            break;
        }
        case Row::Rotation:
            s.text(y, 4, "ROTATION", color);
            s.text(y, 20, rotation_ ? "<  ON  >" : "<  OFF >", color);
            break;
        case Row::Types:
            s.text(y, 4, "BLOCK TYPES", color);
            snprintf(buf, sizeof buf, "<  %d  >", typeCount_);
            s.text(y, 20, buf, color);
            break;
        case Row::Armor: {
            s.text(y, 4, "HARD BLOCK", color);
            char label[12];
            if (ARMOR_VALUES[armorIdx_] == 0) {
                snprintf(label, sizeof label, "OFF");
            }
            else {
                snprintf(label, sizeof label, "EVERY %d", ARMOR_VALUES[armorIdx_]);
            }
            snprintf(buf, sizeof buf, "<  %-9s>", label);
            s.text(y, 20, buf, color);
            break;
        }
        case Row::Difficulty:
            s.text(y, 4, "DIFFICULTY", color);
            s.text(y, 20, levelNames[static_cast<int>(level_)], color);
            break;
        case Row::Chain:
            s.text(y, 4, "CHAIN", color);
            s.text(y, 20, chain_ ? "<  ON  >" : "<  OFF >", color);
            break;
        case Row::Fade:
            s.text(y, 4, "FADE TIME", color);
            s.text(y, 20, levelNames[static_cast<int>(fade_)], color);
            break;
        case Row::Start:
            s.text(y, 4, "START", color);
            break;
        }
        ++y;
    }

    // current mode name
    s.text(12, 4, "MODE:", 0x999999);
    s.text(12, 10, modeName(rules()).c_str(), 0x00E5FF);

    // description
    int d = 14;
    if (mode_ == GameMode::Standard) {
        if (!rotation_) {
            s.text(d++, 4, "- Blocks cannot be rotated.", 0x999999);
            s.text(d++, 4, "- Each block spawns in a random", 0x999999);
            s.text(d++, 4, "  direction (e.g. a standing I).", 0x999999);
        }
        if (typeCount_ < 7) {
            s.text(d++, 4, "- Block types are picked at random", 0x999999);
            s.text(d++, 4, "  every time you start a game.", 0x999999);
        }
        if (ARMOR_VALUES[armorIdx_] > 0) {
            s.text(d++, 4, "- Every N pieces, a hard piece ([])", 0x999999);
            s.text(d++, 4, "  comes. Filling its row only cracks", 0x999999);
            s.text(d++, 4, "  it (::). It breaks at the next clear.", 0x999999);
        }
        if (rotation_ && typeCount_ == 7 && ARMOR_VALUES[armorIdx_] == 0) {
            s.text(d++, 4, "- Standard rules with all 7 blocks.", 0x999999);
        }
    }
    else if (mode_ == GameMode::Bomb) {
        s.text(d++, 4, "- A piece may contain a bomb (*).", 0x999999);
        s.text(d++, 4, "- Clearing its row also clears the", 0x999999);
        s.text(d++, 4, "  rows above and below.", 0x999999);
        s.text(d++, 4, "- A gauge fills as you play; when", 0x999999);
        s.text(d++, 4, "  it is full, a bomb piece comes.", 0x999999);
        if (level_ == BombLevel::Easy) {
            s.text(d++, 4, "- No stone rows.", 0x999999);
        }
        else if (level_ == BombLevel::Normal) {
            s.text(d++, 4, "- 2 stone rows (#) at the start.", 0x999999);
            s.text(d++, 4, "- Only a blast can remove them.", 0x999999);
        }
        else {
            s.text(d++, 4, "- Stone rows (#) also rise from below.", 0x999999);
            s.text(d++, 4, "- Only a blast can remove them.", 0x999999);
        }
        if (chain_) {
            s.text(d++, 4, "- CHAIN: a bomb hit by a blast", 0x999999);
            s.text(d++, 4, "  explodes too, and so on.", 0x999999);
        }
    }
    else {
        char line[48];
        snprintf(line, sizeof line, "- Placed blocks turn invisible after %.1fs.",
            fadeMillis(fade_) / 1000.0);
        s.text(d++, 4, line, 0x999999);
        s.text(d++, 4, "- Remember where they are!", 0x999999);
        s.text(d++, 4, "- The ghost piece is hidden too.", 0x999999);
        s.text(d++, 4, "- The board is shown on game over.", 0x999999);
    }

    // controls
    s.text(23, 4, "Up/Down : select option", 0x666666);
    s.text(24, 4, "Left/Right : change value", 0x666666);
    s.text(25, 4, "Enter / Space : start", 0x666666);
    s.text(26, 4, "Q : quit", 0x666666);
}