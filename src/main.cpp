#include <chrono>
#include <memory>
#include <thread>
#include "Game.h"
#include "Input.h"
#include "Menu.h"
#include "Screen.h"

int main() {
    Screen screen(50, 27);
    if (!screen.ok()) return 1;

    Input input(screen);          // starts the key input thread
    Menu menu;
    std::unique_ptr<Game> game;   // empty = menu screen, set = game screen

    // Throw away queued keys when switching screens
    auto drainInput = [&]() {
        while (input.pop() != Key::None) {}
        };

    bool running = true;
    while (running) {
        if (!game) {
            // ---- menu screen ----
            for (Key k = input.pop(); k != Key::None; k = input.pop()) {
                MenuResult r = menu.handle(k);
                if (r == MenuResult::Quit) {
                    running = false;
                    break;
                }
                if (r == MenuResult::Start) {
                    game = std::make_unique<Game>(menu.rules());
                    drainInput();
                    break;
                }
            }
            if (!game && running) {
                menu.draw(screen);
                screen.present();
            }
        }
        else {
            // ---- game screen ----
            for (Key k = input.pop(); k != Key::None; k = input.pop()) {
                game->handle(k);
            }

            game->update(Game::Clock::now());
            game->draw(screen);
            screen.present();

            if (game->quit()) {
                running = false;
            }
            else if (game->wantsMenu()) {
                game.reset();
                drainInput();
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(16));   // about 60 fps
    }
    return 0;
}