#pragma once
#include <atomic>
#include <deque>
#include <memory>
#include <mutex>
#include "Screen.h"

struct InputState {
    std::mutex m;
    std::deque<Key> q;
    std::atomic<bool> stop{ false };
};

// 별도 스레드에서 키를 계속 읽어서 큐에 쌓아 두는 클래스
class Input {
public:
    explicit Input(Screen& screen);
    ~Input();

    // 쌓인 키 중 가장 오래된 것을 꺼냄. 없으면 Key::None
    Key pop();

private:
    std::shared_ptr<InputState> st_;
};