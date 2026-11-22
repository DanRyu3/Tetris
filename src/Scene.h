#pragma once
#include <chrono>
#include "Screen.h"

// What a scene asks main() to do next
enum class SceneExit { None, ToGame, ToMenu, Quit };

// Abstract base class of every screen (menu, game).
// main() only knows Scene, and the right handle/update/draw is chosen at run time (polymorphism).
class Scene {
public:
    virtual ~Scene() = default;

    virtual void handle(Key k) = 0;                                   // key input
    virtual void update(std::chrono::steady_clock::time_point now) {  // time passes (default: nothing)
        (void)now;
    }
    virtual void draw(Screen& s) const = 0;                           // draw one frame
    virtual SceneExit exitRequest() const = 0;                        // does this scene want to change?
};