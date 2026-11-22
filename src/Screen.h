#pragma once
#include <cstdint>

struct RsactHandle;   // Rsact internal type (forward declaration)

enum class Key {
    None, Left, Right, Down, Up, Space, Enter,
    Hold, Undo, Pause, Quit, Restart, Menu, Other
};

class Screen {
public:
    Screen(int width, int height);
    ~Screen();
    Screen(const Screen&) = delete;
    Screen& operator=(const Screen&) = delete;

    bool ok() const { return h_ != nullptr; }
    int width() const { return w_; }
    int height() const { return h_size_; }

    void put(int row, int col, char32_t ch, uint32_t fg, uint32_t bg);
    void text(int row, int col, const char* s, uint32_t fg = 0xFFFFFF);
    void clear();
    void present();

    // Waits for one key and converts it to a Key (blocking).
    // true on success, false on EOF or error
    bool readKey(Key& out);

private:
    RsactHandle* h_ = nullptr;
    int w_;
    int h_size_;
};