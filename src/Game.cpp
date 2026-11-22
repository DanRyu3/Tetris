#include "Game.h"
#include <algorithm>
#include <cstdio>
#include <utility>

// 0 = empty, 1~7 = I O T S Z J L, 8 = stone
static const uint32_t COLORS[] = {
    0x000000,
    0x00E5FF, 0xFFD600, 0xAA00FF, 0x00E676, 0xFF1744, 0x2979FF, 0xFF9100,
    0x606060
};

static const char TYPE_LETTERS[] = "IOTSZJL";

// ---- balance (change these numbers to tune) ----
static const int UNDO_MAX = 3;            // undos per game
static const int GAUGE_MAX = 12;          // bomb mode: a bomb piece comes when the gauge reaches this
static const int GAUGE_PER_PIECE = 1;     // bomb mode: gauge gained for every placed piece
static const int GAUGE_PER_LINE = 3;      // bomb mode: gauge gained for every filled row
static const int BLAST_BONUS = 100;       // bomb mode: bonus points for every extra row removed by a blast
static const int START_STONE_ROWS = 2;    // bomb mode NORMAL / HARD: stone rows at the start
static const int HARD_RISE_EVERY = 12;    // bomb mode HARD: a stone row rises every N placed pieces

// invisible mode: how long (ms) a placed block stays visible. EASY / NORMAL / HARD
static const int FADE_MS[] = { 15000, 6000, 1500 };
static const int FADE_WARNING_PERCENT = 70;   // the block starts to dim after this part of the time

// make a color darker (ghost piece, used hold, fading blocks)
static uint32_t dim(uint32_t c, int percent) {
    uint32_t r = ((c >> 16) & 0xFF) * percent / 100;
    uint32_t g = ((c >> 8) & 0xFF) * percent / 100;
    uint32_t b = (c & 0xFF) * percent / 100;
    return (r << 16) | (g << 8) | b;
}

// how a shape value is drawn: 2 = bomb, 3 = hard block
static void glyphForShapeValue(int v, char32_t& a, char32_t& b) {
    a = U' ';
    b = U' ';
    if (v == 2) { a = U'*'; b = U'*'; }
    else if (v == 3) { a = U'['; b = U']'; }
}

// how a placed cell is drawn
static void glyphForCell(const Cell& cell, char32_t& a, char32_t& b) {
    a = U' ';
    b = U' ';
    if (cell.color == STONE_COLOR) { a = U'#'; b = U'#'; }
    else if (cell.bomb) { a = U'*'; b = U'*'; }
    else if (cell.armor == 2) { a = U'['; b = U']'; }
    else if (cell.armor == 1) { a = U':'; b = U':'; }
}

int fadeMillis(FadeLevel level) {
    return FADE_MS[static_cast<int>(level)];
}

std::string modeName(const Rules& r) {
    if (r.mode == GameMode::Bomb) {
        std::string s;
        switch (r.bombLevel) {
        case BombLevel::Easy:   s = "BOMB - EASY";   break;
        case BombLevel::Normal: s = "BOMB - NORMAL"; break;
        case BombLevel::Hard:   s = "BOMB - HARD";   break;
        }
        if (r.bombChain) s += " + CHAIN";
        return s;
    }

    if (r.mode == GameMode::Invisible) {
        switch (r.fadeLevel) {
        case FadeLevel::Easy:   return "INVISIBLE - EASY";
        case FadeLevel::Normal: return "INVISIBLE - NORMAL";
        case FadeLevel::Hard:   return "INVISIBLE - HARD";
        }
        return "INVISIBLE";
    }

    std::string s;
    if (!r.allowRotation) s += "NO ROTATION";
    if (r.typeCount < 7) {
        if (!s.empty()) s += " + ";
        s += std::to_string(r.typeCount) + "-BLOCK";
    }
    if (r.armorEvery > 0) {
        if (!s.empty()) s += " + ";
        s += "HARD BLOCK";
    }
    if (s.empty()) s = "CLASSIC";
    return s;
}

Game::Game(Rules rules) : rules_(rules) {
    // modes never mix: the other modes ignore the Standard options
    if (rules_.mode != GameMode::Standard) {
        rules_.allowRotation = true;
        rules_.typeCount = 7;
        rules_.armorEvery = 0;
    }
    if (rules_.mode != GameMode::Bomb) {
        rules_.bombChain = false;
    }
    pickTypes();
    reset();
}

void Game::pickTypes() {
    int n = std::clamp(rules_.typeCount, 1, 7);
    std::vector<int> all = { 0, 1, 2, 3, 4, 5, 6 };

    std::shuffle(all.begin(), all.end(), rng_);
    allowed_.assign(all.begin(), all.begin() + n);
    std::sort(allowed_.begin(), allowed_.end());
}

void Game::reset() {
    board_ = Board{};
    bag_.clear();
    score_ = 0;
    lines_ = 0;
    level_ = 1;
    over_ = false;
    paused_ = false;
    hasHold_ = false;
    holdUsed_ = false;
    gauge_ = 0;
    piecesSinceRise_ = 0;
    armorCount_ = 0;
    boomExtra_ = 0;
    boomChains_ = 0;
    boomUntil_ = Clock::time_point{};
    undosLeft_ = UNDO_MAX;
    hasPrev_ = false;

    // NORMAL / HARD start with stone rows at the bottom
    if (bombMode() && rules_.bombLevel != BombLevel::Easy) {
        board_.fillStoneRows(START_STONE_ROWS);
    }

    rollNext();
    spawn();
    spawnSnap_ = capture();
    lastDrop_ = Clock::now();
}

// bag: shuffle the block types used in this game, take one at a time, refill when empty
int Game::nextFromBag() {
    if (bag_.empty()) {
        for (int t : allowed_) bag_.push_back(t);
        std::shuffle(bag_.begin(), bag_.end(), rng_);
    }
    int t = bag_.back();
    bag_.pop_back();
    return t;
}

// the shape a piece has when it first appears
// rotation on : always the base direction
// rotation off: one of the 4 directions at random (e.g. a standing I)
Shape Game::makeSpawnShape(PieceType t) {
    Shape s = baseShape(t);
    if (!rules_.allowRotation) {
        std::uniform_int_distribution<int> dist(0, 3);
        int turns = dist(rng_);
        for (int i = 0; i < turns; ++i) {
            s = rotateCW(s, t);
        }
    }
    return s;
}

// pick one normal cell of the piece at random and make it a bomb
void Game::placeBombCell(Shape& s) {
    std::vector<std::pair<int, int>> cells;
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            if (s[i][j] == 1) cells.push_back({ i, j });
        }
    }
    if (cells.empty()) return;

    std::uniform_int_distribution<size_t> dist(0, cells.size() - 1);
    std::pair<int, int> pick = cells[dist(rng_)];
    s[pick.first][pick.second] = 2;
}

// the whole piece becomes a hard block
void Game::hardenShape(Shape& s) {
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            if (s[i][j] == 1) s[i][j] = 3;
        }
    }
}

void Game::rollNext() {
    next_ = nextFromBag();
    nextCells_ = makeSpawnShape(static_cast<PieceType>(next_));

    // bomb mode: gauge is full, this piece comes with a bomb (shown in the NEXT box)
    if (bombMode() && gauge_ >= GAUGE_MAX) {
        gauge_ -= GAUGE_MAX;
        placeBombCell(nextCells_);
    }

    // standard mode: cooldown is over, this piece is a hard piece
    if (rules_.armorEvery > 0 && armorCount_ >= rules_.armorEvery) {
        armorCount_ -= rules_.armorEvery;
        hardenShape(nextCells_);
    }
}

void Game::takeNext(PieceType& t, Shape& cells) {
    t = static_cast<PieceType>(next_);
    cells = nextCells_;
    rollNext();
}

void Game::placeNew(PieceType t, const Shape& cells) {
    cur_ = Piece{};
    cur_.type = t;
    cur_.cells = cells;
    cur_.spawnCells = cells;
    cur_.x = 3;
    cur_.y = 0;

    if (board_.collides(cur_)) over_ = true;    // game over if the spawn spot is blocked
}

void Game::spawn() {
    PieceType t;
    Shape cells;
    takeNext(t, cells);
    holdUsed_ = false;                          // hold can be used again for the new piece
    placeNew(t, cells);
}

bool Game::tryMove(int dx, int dy) {
    Piece t = cur_;
    t.x += dx;
    t.y += dy;
    if (board_.collides(t)) return false;
    cur_ = t;
    return true;
}

void Game::tryRotate() {
    if (!rules_.allowRotation) return;          // No Rotation

    Piece t = cur_;
    t.cells = rotateCW(cur_.cells, cur_.type);

    // simple wall kick: try shifting left and right a little
    for (int kick : {0, -1, 1, -2, 2}) {
        Piece k = t;
        k.x += kick;
        if (!board_.collides(k)) {
            cur_ = k;
            return;
        }
    }
}

void Game::hardDrop() {
    while (tryMove(0, 1)) score_ += 2;
    lockAndClear();
}

void Game::holdPiece() {
    if (holdUsed_) return;                                  // already used for this piece
    if (hasHold_ && heldType_ == cur_.type) return;         // same type: nothing happens, not used up

    PieceType curType = cur_.type;
    Shape curCells = cur_.spawnCells;                       // keep the spawn shape (direction, bomb, hard block)

    if (!hasHold_) {
        // hold is empty: store the piece, the next piece comes out
        heldType_ = curType;
        heldCells_ = curCells;
        hasHold_ = true;

        PieceType t;
        Shape cells;
        takeNext(t, cells);
        placeNew(t, cells);
    }
    else {
        // swap with the held piece
        PieceType heldType = heldType_;
        Shape heldCells = heldCells_;
        heldType_ = curType;
        heldCells_ = curCells;

        placeNew(heldType, heldCells);
    }

    holdUsed_ = true;
    lastDrop_ = Clock::now();                               // the new piece does not fall right away
}

void Game::lockAndClear() {
    board_.lock(cur_, Clock::now());
    ClearResult res = board_.clearLines(bombMode() && rules_.bombChain);

    // score: filled rows by the usual table, rows removed by a blast give a bonus
    static const int points[] = { 0, 100, 300, 500, 800 };
    int idx = std::min(res.completed, 4);
    int extra = res.removed - res.completed;
    score_ += (points[idx] + extra * BLAST_BONUS) * level_;
    lines_ += res.removed;
    level_ = 1 + lines_ / 10;

    if (res.bombs > 0) {
        boomExtra_ = extra;
        boomChains_ = res.chains;
        boomUntil_ = Clock::now() + std::chrono::milliseconds(1500);
    }

    if (bombMode()) {
        gauge_ += GAUGE_PER_PIECE + res.completed * GAUGE_PER_LINE;

        // HARD: a stone row rises from the bottom every few pieces
        if (rules_.bombLevel == BombLevel::Hard) {
            ++piecesSinceRise_;
            if (piecesSinceRise_ >= HARD_RISE_EVERY) {
                piecesSinceRise_ = 0;
                if (!board_.pushStoneRow()) over_ = true;   // pushed out of the top
            }
        }
    }

    if (rules_.armorEvery > 0) ++armorCount_;

    // the state when the locked piece appeared becomes the undo point
    prevSnap_ = spawnSnap_;
    hasPrev_ = true;

    spawn();
    spawnSnap_ = capture();
}

Game::Snapshot Game::capture() const {
    Snapshot s;
    s.board = board_;
    s.cur = cur_;
    s.bag = bag_;
    s.next = next_;
    s.nextCells = nextCells_;
    s.score = score_;
    s.lines = lines_;
    s.level = level_;
    s.hasHold = hasHold_;
    s.holdUsed = holdUsed_;
    s.heldType = heldType_;
    s.heldCells = heldCells_;
    s.gauge = gauge_;
    s.piecesSinceRise = piecesSinceRise_;
    s.armorCount = armorCount_;
    return s;
}

void Game::restore(const Snapshot& s) {
    board_ = s.board;
    cur_ = s.cur;
    bag_ = s.bag;
    next_ = s.next;
    nextCells_ = s.nextCells;
    score_ = s.score;
    lines_ = s.lines;
    level_ = s.level;
    hasHold_ = s.hasHold;
    holdUsed_ = s.holdUsed;
    heldType_ = s.heldType;
    heldCells_ = s.heldCells;
    gauge_ = s.gauge;
    piecesSinceRise_ = s.piecesSinceRise;
    armorCount_ = s.armorCount;

    over_ = false;
    paused_ = false;
    boomExtra_ = 0;
    boomChains_ = 0;
    boomUntil_ = Clock::time_point{};
    lastDrop_ = Clock::now();
}

// take back the last placed piece: the board goes back and the piece appears again
void Game::undo() {
    if (undosLeft_ <= 0 || !hasPrev_) return;

    restore(prevSnap_);
    spawnSnap_ = prevSnap_;
    hasPrev_ = false;               // only one step back at a time
    --undosLeft_;
}

int Game::dropIntervalMs() const {
    return std::max(80, 800 - (level_ - 1) * 70);
}

int Game::ghostY() const {
    Piece g = cur_;
    while (true) {
        Piece t = g;
        t.y += 1;
        if (board_.collides(t)) break;
        g = t;
    }
    return g.y;
}

void Game::handle(Key k) {
    if (k == Key::Quit) { quit_ = true; return; }
    if (k == Key::Menu) { wantsMenu_ = true; return; }
    if (k == Key::Restart) { reset(); return; }          // restart with the same block types
    if (k == Key::Pause) {
        if (!over_) {
            paused_ = !paused_;
            if (!paused_) lastDrop_ = Clock::now();   // do not drop right after resuming
        }
        return;
    }
    if (k == Key::Undo) {                                // also works after game over
        if (!paused_) undo();
        return;
    }
    if (paused_ || over_) return;

    switch (k) {
    case Key::Left:  tryMove(-1, 0); break;
    case Key::Right: tryMove(1, 0);  break;
    case Key::Down:  if (tryMove(0, 1)) ++score_; break;
    case Key::Up:    tryRotate();    break;
    case Key::Space: hardDrop();     break;
    case Key::Hold:  holdPiece();    break;
    default: break;
    }
}

void Game::update(Clock::time_point now) {
    if (paused_ || over_) return;

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastDrop_).count();
    if (elapsed >= dropIntervalMs()) {
        lastDrop_ = now;
        if (!tryMove(0, 1)) lockAndClear();     // lock when it cannot go down any more
    }
}

void Game::draw(Screen& s) const {
    s.clear();

    const int top = 1;
    const int left = 2;
    const int hud = left + BW * 2 + 3;          // left edge of the info area
    const Clock::time_point now = Clock::now();

    // one tetris cell = 2 columns. a, b = the two characters drawn in it
    auto block = [&](int row, int col, uint32_t color, char32_t a = U' ', char32_t b = U' ') {
        s.put(row, col, a, 0x000000, color);
        s.put(row, col + 1, b, 0x000000, color);
        };

    s.text(0, left, "TETRIS");
    s.text(0, left + 8, modeName(rules_).c_str(), 0x00E5FF);

    // border
    for (int r = 0; r < BH; ++r) {
        s.put(top + r, left - 1, U'|', 0x888888, 0x000000);
        s.put(top + r, left + BW * 2, U'|', 0x888888, 0x000000);
    }
    for (int c = -1; c <= BW * 2; ++c) {
        s.put(top + BH, left + c, U'-', 0x888888, 0x000000);
    }

    // 1) ghost piece (drawn first so other blocks cover it). hidden in invisible mode
    if (!over_) {
        int gy = ghostY();
        uint32_t ghostColor = dim(COLORS[static_cast<int>(cur_.type) + 1], 40);
        for (int i = 0; i < N; ++i) {
            for (int j = 0; j < N; ++j) {
                if (!cur_.cells[i][j]) continue;
                int r = gy + i;
                int c = cur_.x + j;
                if (r < 0 || r >= BH || c < 0 || c >= BW) continue;
                block(top + r, left + c * 2, ghostColor);
            }
        }
    }

    // 2) placed cells
    const int fade = fadeMillis(rules_.fadeLevel);
    for (int r = 0; r < BH; ++r) {
        for (int c = 0; c < BW; ++c) {
            const Cell& cell = board_.at(r, c);
            if (!cell.color) continue;

            uint32_t color = COLORS[cell.color];

            // invisible mode: placed blocks fade out (everything is shown after game over)
            if (invisibleMode() && !over_) {
                long long age = std::chrono::duration_cast<std::chrono::milliseconds>(now - cell.lockedAt).count();
                if (age >= fade) continue;                                  // invisible now
                if (age >= static_cast<long long>(fade) * FADE_WARNING_PERCENT / 100) {
                    color = dim(color, 45);                                 // about to disappear
                }
            }

            char32_t a, b;
            glyphForCell(cell, a, b);
            block(top + r, left + c * 2, color, a, b);
        }
    }

    // 3) the falling piece
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            if (!cur_.cells[i][j]) continue;
            int r = cur_.y + i;
            int c = cur_.x + j;
            if (r < 0 || r >= BH || c < 0 || c >= BW) continue;

            char32_t a, b;
            glyphForShapeValue(cur_.cells[i][j], a, b);
            block(top + r, left + c * 2, COLORS[static_cast<int>(cur_.type) + 1], a, b);
        }
    }

    // score info
    char buf[48];
    snprintf(buf, sizeof buf, "SCORE %d", score_);
    s.text(top + 1, hud, buf);
    snprintf(buf, sizeof buf, "LINES %d", lines_);
    s.text(top + 2, hud, buf);
    snprintf(buf, sizeof buf, "LEVEL %d", level_);
    s.text(top + 3, hud, buf);

    // next piece (shown in the direction it will really appear, special cells included)
    s.text(top + 5, hud, "NEXT");
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            if (!nextCells_[i][j]) continue;

            char32_t a, b;
            glyphForShapeValue(nextCells_[i][j], a, b);
            block(top + 6 + i, hud + j * 2, COLORS[next_ + 1], a, b);
        }
    }

    // hold (dimmed when already used for this piece)
    s.text(top + 11, hud, holdUsed_ ? "HOLD (used)" : "HOLD");
    if (hasHold_) {
        uint32_t hc = COLORS[static_cast<int>(heldType_) + 1];
        if (holdUsed_) hc = dim(hc, 40);
        for (int i = 0; i < N; ++i) {
            for (int j = 0; j < N; ++j) {
                if (!heldCells_[i][j]) continue;

                char32_t a, b;
                glyphForShapeValue(heldCells_[i][j], a, b);
                block(top + 12 + i, hud + j * 2, hc, a, b);
            }
        }
    }

    // undo
    snprintf(buf, sizeof buf, "UNDO %d/%d", undosLeft_, UNDO_MAX);
    s.text(top + 16, hud, buf, (undosLeft_ > 0 && hasPrev_) ? 0xFFFFFF : 0x777777);

    // block types of this game (only when not all 7)
    if (allowed_.size() < 7) {
        std::string types = "TYPES:";
        for (int t : allowed_) {
            types += ' ';
            types += TYPE_LETTERS[t];
        }
        s.text(top + 17, hud, types.c_str(), 0xFFD600);
    }

    // invisible mode: fade time
    if (invisibleMode()) {
        snprintf(buf, sizeof buf, "FADE %.1fs", fade / 1000.0);
        s.text(top + 17, hud, buf, 0xB39DDB);
    }

    // hard piece cooldown (standard mode)
    if (rules_.armorEvery > 0) {
        int filled = std::min(armorCount_, rules_.armorEvery);
        std::string bar = "HARD [";
        for (int i = 0; i < rules_.armorEvery; ++i) {
            bar += (i < filled) ? '#' : '.';
        }
        bar += ']';
        s.text(top + 18, hud, bar.c_str(), 0x9FA8DA);
    }

    // bomb gauge and blast message
    if (bombMode()) {
        int filled = std::min(gauge_, GAUGE_MAX) * 12 / GAUGE_MAX;
        std::string bar = "BOMB [";
        for (int i = 0; i < 12; ++i) {
            bar += (i < filled) ? '#' : '.';
        }
        bar += ']';
        s.text(top + 17, hud, bar.c_str(), 0xFF9100);

        if (now < boomUntil_) {
            if (boomChains_ > 0) {
                snprintf(buf, sizeof buf, "BOOM! +%d rows CHAIN x%d", boomExtra_, boomChains_);
            }
            else if (boomExtra_ > 0) {
                snprintf(buf, sizeof buf, "BOOM! +%d rows", boomExtra_);
            }
            else {
                snprintf(buf, sizeof buf, "BOOM!");
            }
            s.text(top + 18, hud, buf, 0xFF1744);
        }
    }

    // controls
    s.text(top + 19, hud, "Left/Right: move", 0x999999);
    s.text(top + 20, hud, rules_.allowRotation ? "Up: rotate" : "Up: (rotation off)", 0x999999);
    s.text(top + 21, hud, "Down: soft drop", 0x999999);
    s.text(top + 22, hud, "Space: hard drop", 0x999999);
    s.text(top + 23, hud, "C: hold  Z: undo", 0x999999);
    s.text(top + 24, hud, "P: pause  M: menu", 0x999999);
    s.text(top + 25, hud, "R: restart  Q: quit", 0x999999);

    // status messages in the middle of the board
    if (paused_) {
        s.text(top + 9, left + 7, "PAUSED", 0xFFD600);
    }
    if (over_) {
        s.text(top + 9, left + 5, "GAME OVER", 0xFF1744);
        s.text(top + 10, left + 5, "Z: undo", 0xFFFFFF);
        s.text(top + 11, left + 5, "R: restart", 0xFFFFFF);
        s.text(top + 12, left + 5, "M: menu", 0xFFFFFF);
    }
}