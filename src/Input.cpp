#include "Input.h"
#include <thread>

Input::Input(Screen& screen) : st_(std::make_shared<InputState>()) {
    std::shared_ptr<InputState> st = st_;
    Screen* sc = &screen;

    std::thread([st, sc]() {
        while (!st->stop) {
            Key k = Key::Other;
            if (!sc->readKey(k)) break;      // 키가 올 때까지 여기서 멈춰 있음
            if (st->stop) break;
            if (k == Key::Other) continue;

            std::lock_guard<std::mutex> lock(st->m);
            if (st->q.size() < 64) st->q.push_back(k);
        }
        }).detach();
}

Input::~Input() {
    st_->stop = true;
}

Key Input::pop() {
    std::lock_guard<std::mutex> lock(st_->m);
    if (st_->q.empty()) return Key::None;
    Key k = st_->q.front();
    st_->q.pop_front();
    return k;
}