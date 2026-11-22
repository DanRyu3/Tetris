#include "Screen.h"
#include <cctype>

extern "C" {
#include "rsact.h"
}

Screen::Screen(int width, int height) : w_(width), h_size_(height) {
    h_ = rsact_create(static_cast<unsigned short>(width),
        static_cast<unsigned short>(height));
}

Screen::~Screen() {
    if (h_) rsact_destroy(h_);
}

void Screen::put(int row, int col, char32_t ch, uint32_t fg, uint32_t bg) {
    if (!h_ || row < 0 || col < 0 || row >= h_size_ || col >= w_) return;
    rsact_set_cell(h_,
        static_cast<unsigned short>(row),
        static_cast<unsigned short>(col),
        static_cast<unsigned int>(ch), fg, bg, 0);
}

void Screen::text(int row, int col, const char* s, uint32_t fg) {
    for (; *s; ++s, ++col) {
        put(row, col, static_cast<char32_t>(*s), fg, 0x000000);
    }
}

void Screen::clear() {
    for (int r = 0; r < h_size_; ++r) {
        for (int c = 0; c < w_; ++c) {
            put(r, c, U' ', 0xFFFFFF, 0x000000);
        }
    }
}

void Screen::present() {
    if (h_) rsact_render(h_);
}

bool Screen::readKey(Key& out) {
    if (!h_) return false;

    RsactKeyEvent ev{};
    int32_t rc = rsact_poll_key(h_, &ev);    // blocks until a key arrives
    if (rc != 1) return false;

    out = Key::Other;

    if (ev.kind == 2) {                      // special keys (arrows, Enter ...)
        switch (ev.code) {
        case RSACT_KEY_LEFT:  out = Key::Left;  break;
        case RSACT_KEY_RIGHT: out = Key::Right; break;
        case RSACT_KEY_DOWN:  out = Key::Down;  break;
        case RSACT_KEY_UP:    out = Key::Up;    break;
        case RSACT_KEY_ENTER: out = Key::Enter; break;   // Esc arrives with the same value
        default: break;
        }
    }
    else if (ev.kind == 1) {               // normal character keys
        if (ev.code == 32) {
            out = Key::Space;
        }
        else if (ev.code < 128) {
            switch (std::tolower(static_cast<int>(ev.code))) {
            case 'a': out = Key::Left;    break;
            case 'd': out = Key::Right;   break;
            case 's': out = Key::Down;    break;
            case 'w': out = Key::Up;      break;
            case 'c': out = Key::Hold;    break;
            case 'z': out = Key::Undo;    break;
            case 'u': out = Key::Undo;    break;
            case 'p': out = Key::Pause;   break;
            case 'q': out = Key::Quit;    break;
            case 'r': out = Key::Restart; break;
            case 'm': out = Key::Menu;    break;
            default: break;
            }
        }
    }
    return true;
}