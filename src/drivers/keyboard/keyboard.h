#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdbool.h>

typedef enum {
    KEY_NONE = 0,

    KEY_ESC,
    KEY_1,
    KEY_2,
    KEY_3,
    KEY_4,
    KEY_5,
    KEY_6,
    KEY_7,
    KEY_8,
    KEY_9,
    KEY_0,
    KEY_MINUS,
    KEY_EQUALS,
    KEY_BACKSPACE,
    KEY_TAB,

    KEY_Q,
    KEY_W,
    KEY_E,
    KEY_R,
    KEY_T,
    KEY_Y,
    KEY_U,
    KEY_I,
    KEY_O,
    KEY_P,
    KEY_LBRACKET,
    KEY_RBRACKET,
    KEY_ENTER,
    KEY_LCTRL,

    KEY_A,
    KEY_S,
    KEY_D,
    KEY_F,
    KEY_G,
    KEY_H,
    KEY_J,
    KEY_K,
    KEY_L,
    KEY_SEMICOLON,
    KEY_APOSTROPHE,
    KEY_GRAVE,

    KEY_LSHIFT,
    KEY_BACKSLASH,

    KEY_Z,
    KEY_X,
    KEY_C,
    KEY_V,
    KEY_B,
    KEY_N,
    KEY_M,
    KEY_COMMA,
    KEY_PERIOD,
    KEY_SLASH,
    KEY_RSHIFT,

    KEY_NUMPAD_STAR,
    KEY_LALT,
    KEY_SPACE,
    KEY_CAPSLOCK,

    KEY_F1,
    KEY_F2,
    KEY_F3,
    KEY_F4,
    KEY_F5,
    KEY_F6,
    KEY_F7,
    KEY_F8,
    KEY_F9,
    KEY_F10,

    KEY_NUMLOCK,
    KEY_SCROLLLOCK,

    KEY_F11,
    KEY_F12,

    KEY_RCTRL,
    KEY_RALT,
    KEY_UP,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_HOME,
    KEY_END,
    KEY_PGUP,
    KEY_PGDN,
    KEY_INSERT,
    KEY_DELETE,
    KEY_NUMPAD_ENTER,
    KEY_NUMPAD_SLASH,
    KEY_LGUI,
    KEY_RGUI,
    KEY_MENU,
    KEY_PRINTSCREEN,

    KEY_PAUSE,

    KEY_COUNT
} KeyCode;

void keyboard_init(void);
KeyCode scancode_to_keycode(unsigned char scancode);
KeyCode extended_scancode_to_keycode(unsigned char scancode);
char keycode_to_char(KeyCode key);
KeyCode char_to_keycode(char c);

unsigned char get_scancode();
unsigned char strip_key(unsigned char key);
void keyboard_handler();
void keyboard_irq_handler();
bool is_key_held(unsigned char key);

#endif
