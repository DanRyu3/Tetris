#include <chrono>
#include <memory>
#include <thread>
#include "Game.h"
#include "Input.h"
#include "Menu.h"
#include "Scene.h"
#include "Screen.h"

int main() {
    Screen screen(50, 27);
    if (!screen.ok()) return 1;

    Input input(screen);          // starts the key input thread
    Menu menu;                    // the menu scene (always exists)
    std::unique_ptr<Game> game;   // the game scene (empty while the menu is shown)

    // Throw away queued keys when switching scenes
    auto drainInput = [&]() {
        while (input.pop() != Key::None) {}
        };

    bool running = true;
    while (running) {
        // Polymorphism: scene points to either the menu or the game.
        // The same calls below run the menu's or the game's own version.
        Scene* scene = game ? static_cast<Scene*>(game.get())
            : static_cast<Scene*>(&menu);

        for (Key k = input.pop(); k != Key::None; k = input.pop()) {
            scene->handle(k);
            if (scene->exitRequest() != SceneExit::None) break;
        }

        scene->update(std::chrono::steady_clock::now());
        scene->draw(screen);
        screen.present();

        // What does the scene want next?
        switch (scene->exitRequest()) {
        case SceneExit::ToGame:
            game = std::make_unique<Game>(menu.rules());
            menu.clearRequest();
            drainInput();
            break;
        case SceneExit::ToMenu:
            game.reset();
            drainInput();
            break;
        case SceneExit::Quit:
            running = false;
            break;
        case SceneExit::None:
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(16));   // about 60 fps
    }
    return 0;
}