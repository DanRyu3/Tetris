#pragma once
#include <chrono>
#include <random>
#include <string>
#include <vector>
#include "Board.h"
#include "Piece.h"
#include "Scene.h"
#include "Screen.h"

enum class GameMode { Standard, Bomb, Invisible };
enum class BombLevel { Easy, Normal, Hard };
enum class FadeLevel { Easy, Normal, Hard };

// Game rules. Each mode has its own options and the modes are never mixed.
struct Rules {
    GameMode  mode = GameMode::Standard;

    // Standard mode options
    bool allowRotation = true;      // false = No Rotation (blocks spawn in a random direction)
    int  typeCount = 7;             // 1~7 block types (less than 7 = N-Block)
    int  armorEvery = 0;            // 0 = off. otherwise a hard piece comes every N placed pieces

    // Bomb mode options
    BombLevel bombLevel = BombLevel::Normal;
    bool      bombChain = false;    // a bomb hit by a blast explodes too

    // Invisible mode options
    FadeLevel fadeLevel = FadeLevel::Normal;
};

// Name shown in the menu and the HUD (CLASSIC, NO ROTATION + HARD BLOCK, BOMB - HARD + CHAIN, INVISIBLE - HARD ...)
std::string modeName(const Rules& r);

// Invisible mode: how long (ms) a placed block stays visible
int fadeMillis(FadeLevel level);

// The game screen. It is one kind of Scene (inheritance).
class Game : public Scene {
public:
    using Clock = std::chrono::steady_clock;

    explicit Game(Rules rules);

    // Scene interface (overridden virtual functions)
    void handle(Key k) override;
    void update(Clock::time_point now) override;
    void draw(Screen& s) const override;
    SceneExit exitRequest() const override {
        if (quit_) return SceneExit::Quit;           // quit the program
        if (wantsMenu_) return SceneExit::ToMenu;    // go back to the menu
        return SceneExit::None;
    }

    bool quit() const { return quit_; }
    bool wantsMenu() const { return wantsMenu_; }

private:
    // state saved every time a piece appears (used by undo)
    struct Snapshot {
        Board board;
        Piece cur;
        std::vector<int> bag;
        int   next = 0;
        Shape nextCells{};
        int   score = 0;
        int   lines = 0;
        int   level = 1;
        bool      hasHold = false;
        bool      holdUsed = false;
        PieceType heldType = PieceType::I;
        Shape     heldCells{};
        int   gauge = 0;
        int   piecesSinceRise = 0;
        int   armorCount = 0;
    };

    bool bombMode() const { return rules_.mode == GameMode::Bomb; }
    bool invisibleMode() const { return rules_.mode == GameMode::Invisible; }

    void pickTypes();                                 // choose the block types for this game
    void reset();
    void rollNext();                                  // decide the next piece (type and shape)
    void takeNext(PieceType& t, Shape& cells);        // take the next piece and roll a new one
    Shape makeSpawnShape(PieceType t);                // spawn shape (random direction when rotation is off)
    void placeBombCell(Shape& s);                     // turn one cell of the shape into a bomb
    void hardenShape(Shape& s);                       // turn every cell of the shape into a hard block
    void spawn();
    void placeNew(PieceType t, const Shape& cells);
    bool tryMove(int dx, int dy);
    void tryRotate();
    void hardDrop();
    void holdPiece();
    void lockAndClear();
    void undo();
    Snapshot capture() const;
    void restore(const Snapshot& s);
    int  nextFromBag();
    int  dropIntervalMs() const;
    int  ghostY() const;

    Rules rules_;
    Board board_;
    Piece cur_;
    std::vector<int> allowed_;
    std::vector<int> bag_;
    std::mt19937 rng_{ std::random_device{}() };
    int   next_ = 0;
    Shape nextCells_{};
    int  score_ = 0;
    int  lines_ = 0;
    int  level_ = 1;
    bool over_ = false;
    bool paused_ = false;
    bool quit_ = false;
    bool wantsMenu_ = false;
    Clock::time_point lastDrop_;

    // hold
    bool      hasHold_ = false;
    bool      holdUsed_ = false;
    PieceType heldType_ = PieceType::I;
    Shape     heldCells_{};

    // undo
    Snapshot spawnSnap_;           // state when the current piece appeared
    Snapshot prevSnap_;            // state when the previous piece appeared
    bool     hasPrev_ = false;     // prevSnap_ can be restored
    int      undosLeft_ = 0;

    // bomb mode
    int  gauge_ = 0;               // fills while playing. a bomb piece comes when it is full
    int  piecesSinceRise_ = 0;     // HARD: pieces placed since the last stone row rose
    int  boomExtra_ = 0;           // rows removed by the last blast (for the message)
    int  boomChains_ = 0;          // chain explosions in the last blast
    Clock::time_point boomUntil_{};

    // hard block (standard mode)
    int  armorCount_ = 0;          // pieces placed since the last hard piece
};