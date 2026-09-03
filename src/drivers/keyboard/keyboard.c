#include "drivers/keyboard/keyboard.h"
#include "portio.h"
#include "drivers/serial/serialController.h"
#include "drivers/fb/fbController.h"
#include "util/rb/ringBuffer.h"
#include <stdbool.h>

bool keys_down[KEY_COUNT] = {false};
bool capslock = false;
bool scrollock = false;
bool numlock = false;

ringBuffer g_rb;

void keyboard_init(void)
{
  rb_init(&g_rb);
}

KeyCode scancode_to_keycode(unsigned char scancode)
{
    switch (scancode) {
        case 0x01: return KEY_ESC;
        case 0x02: return KEY_1;
        case 0x03: return KEY_2;
        case 0x04: return KEY_3;
        case 0x05: return KEY_4;
        case 0x06: return KEY_5;
        case 0x07: return KEY_6;
        case 0x08: return KEY_7;
        case 0x09: return KEY_8;
        case 0x0A: return KEY_9;
        case 0x0B: return KEY_0;
        case 0x0C: return KEY_MINUS;
        case 0x0D: return KEY_EQUALS;
        case 0x0E: return KEY_BACKSPACE;
        case 0x0F: return KEY_TAB;

        case 0x10: return KEY_Q;
        case 0x11: return KEY_W;
        case 0x12: return KEY_E;
        case 0x13: return KEY_R;
        case 0x14: return KEY_T;
        case 0x15: return KEY_Y;
        case 0x16: return KEY_U;
        case 0x17: return KEY_I;
        case 0x18: return KEY_O;
        case 0x19: return KEY_P;
        case 0x1A: return KEY_LBRACKET;
        case 0x1B: return KEY_RBRACKET;
        case 0x1C: return KEY_ENTER;
        case 0x1D: return KEY_LCTRL;

        case 0x1E: return KEY_A;
        case 0x1F: return KEY_S;
        case 0x20: return KEY_D;
        case 0x21: return KEY_F;
        case 0x22: return KEY_G;
        case 0x23: return KEY_H;
        case 0x24: return KEY_J;
        case 0x25: return KEY_K;
        case 0x26: return KEY_L;
        case 0x27: return KEY_SEMICOLON;
        case 0x28: return KEY_APOSTROPHE;
        case 0x29: return KEY_GRAVE;

        case 0x2A: return KEY_LSHIFT;
        case 0x2B: return KEY_BACKSLASH;

        case 0x2C: return KEY_Z;
        case 0x2D: return KEY_X;
        case 0x2E: return KEY_C;
        case 0x2F: return KEY_V;
        case 0x30: return KEY_B;
        case 0x31: return KEY_N;
        case 0x32: return KEY_M;
        case 0x33: return KEY_COMMA;
        case 0x34: return KEY_PERIOD;
        case 0x35: return KEY_SLASH;
        case 0x36: return KEY_RSHIFT;

        case 0x37: return KEY_NUMPAD_STAR;
        case 0x38: return KEY_LALT;
        case 0x39: return KEY_SPACE;
        case 0x3A: return KEY_CAPSLOCK;

        case 0x3B: return KEY_F1;
        case 0x3C: return KEY_F2;
        case 0x3D: return KEY_F3;
        case 0x3E: return KEY_F4;
        case 0x3F: return KEY_F5;
        case 0x40: return KEY_F6;
        case 0x41: return KEY_F7;
        case 0x42: return KEY_F8;
        case 0x43: return KEY_F9;
        case 0x44: return KEY_F10;

        case 0x45: return KEY_NUMLOCK;
        case 0x46: return KEY_SCROLLLOCK;

        case 0x57: return KEY_F11;
        case 0x58: return KEY_F12;

        default: return KEY_NONE;
    }
}

KeyCode extended_scancode_to_keycode(unsigned char scancode)
{
    switch (scancode) {
        case 0x1C: return KEY_NUMPAD_ENTER;
        case 0x1D: return KEY_RCTRL;
        case 0x35: return KEY_NUMPAD_SLASH;
        case 0x37: return KEY_PRINTSCREEN;
        case 0x38: return KEY_RALT;

        case 0x47: return KEY_HOME;
        case 0x48: return KEY_UP;
        case 0x49: return KEY_PGUP;
        case 0x4B: return KEY_LEFT;
        case 0x4D: return KEY_RIGHT;
        case 0x4F: return KEY_END;
        case 0x50: return KEY_DOWN;
        case 0x51: return KEY_PGDN;
        case 0x52: return KEY_INSERT;
        case 0x53: return KEY_DELETE;

        case 0x5B: return KEY_LGUI;
        case 0x5C: return KEY_RGUI;
        case 0x5D: return KEY_MENU;

        default: return KEY_NONE;
    }
}

char keycode_to_char(KeyCode key)
{
    bool shift = keys_down[KEY_RSHIFT] || keys_down[KEY_LSHIFT];
    
    bool uppercase = shift ^ capslock;

    switch (key) {
        case KEY_A: return uppercase ? 'A' : 'a';
        case KEY_B: return uppercase ? 'B' : 'b';
        case KEY_C: return uppercase ? 'C' : 'c';
        case KEY_D: return uppercase ? 'D' : 'd';
        case KEY_E: return uppercase ? 'E' : 'e';
        case KEY_F: return uppercase ? 'F' : 'f';
        case KEY_G: return uppercase ? 'G' : 'g';
        case KEY_H: return uppercase ? 'H' : 'h';
        case KEY_I: return uppercase ? 'I' : 'i';
        case KEY_J: return uppercase ? 'J' : 'j';
        case KEY_K: return uppercase ? 'K' : 'k';
        case KEY_L: return uppercase ? 'L' : 'l';
        case KEY_M: return uppercase ? 'M' : 'm';
        case KEY_N: return uppercase ? 'N' : 'n';
        case KEY_O: return uppercase ? 'O' : 'o';
        case KEY_P: return uppercase ? 'P' : 'p';
        case KEY_Q: return uppercase ? 'Q' : 'q';
        case KEY_R: return uppercase ? 'R' : 'r';
        case KEY_S: return uppercase ? 'S' : 's';
        case KEY_T: return uppercase ? 'T' : 't';
        case KEY_U: return uppercase ? 'U' : 'u';
        case KEY_V: return uppercase ? 'V' : 'v';
        case KEY_W: return uppercase ? 'W' : 'w';
        case KEY_X: return uppercase ? 'X' : 'x';
        case KEY_Y: return uppercase ? 'Y' : 'y';
        case KEY_Z: return uppercase ? 'Z' : 'z';

        case KEY_1: return shift ? '!' : '1';
        case KEY_2: return shift ? '@' : '2';
        case KEY_3: return shift ? '#' : '3';
        case KEY_4: return shift ? '$' : '4';
        case KEY_5: return shift ? '%' : '5';
        case KEY_6: return shift ? '^' : '6';
        case KEY_7: return shift ? '&' : '7';
        case KEY_8: return shift ? '*' : '8';
        case KEY_9: return shift ? '(' : '9';
        case KEY_0: return shift ? ')' : '0';

        case KEY_MINUS:
            return shift ? '_' : '-';

        case KEY_EQUALS:
            return shift ? '+' : '=';

        case KEY_LBRACKET:
            return shift ? '{' : '[';

        case KEY_RBRACKET:
            return shift ? '}' : ']';

        case KEY_SEMICOLON:
            return shift ? ':' : ';';

        case KEY_APOSTROPHE:
            return shift ? '"' : '\'';

        case KEY_GRAVE:
            return shift ? '~' : '`';

        case KEY_BACKSLASH:
            return shift ? '|' : '\\';

        case KEY_COMMA:
            return shift ? '<' : ',';

        case KEY_PERIOD:
            return shift ? '>' : '.';

        case KEY_SLASH:
            return shift ? '?' : '/';

        case KEY_NUMPAD_SLASH:
            return '/';

        case KEY_NUMPAD_ENTER:
            return '\n';

        case KEY_SPACE:
            return ' ';

        case KEY_TAB:
            return '\t';

        case KEY_ENTER:
            return '\n';
        
        case KEY_BACKSPACE:
            return '\b';

        default:
            return '\0';
    }
}

KeyCode char_to_keycode(char c)
{
    switch (c) {
        case 'a': case 'A': return KEY_A;
        case 'b': case 'B': return KEY_B;
        case 'c': case 'C': return KEY_C;
        case 'd': case 'D': return KEY_D;
        case 'e': case 'E': return KEY_E;
        case 'f': case 'F': return KEY_F;
        case 'g': case 'G': return KEY_G;
        case 'h': case 'H': return KEY_H;
        case 'i': case 'I': return KEY_I;
        case 'j': case 'J': return KEY_J;
        case 'k': case 'K': return KEY_K;
        case 'l': case 'L': return KEY_L;
        case 'm': case 'M': return KEY_M;
        case 'n': case 'N': return KEY_N;
        case 'o': case 'O': return KEY_O;
        case 'p': case 'P': return KEY_P;
        case 'q': case 'Q': return KEY_Q;
        case 'r': case 'R': return KEY_R;
        case 's': case 'S': return KEY_S;
        case 't': case 'T': return KEY_T;
        case 'u': case 'U': return KEY_U;
        case 'v': case 'V': return KEY_V;
        case 'w': case 'W': return KEY_W;
        case 'x': case 'X': return KEY_X;
        case 'y': case 'Y': return KEY_Y;
        case 'z': case 'Z': return KEY_Z;

        case '1': case '!': return KEY_1;
        case '2': case '@': return KEY_2;
        case '3': case '#': return KEY_3;
        case '4': case '$': return KEY_4;
        case '5': case '%': return KEY_5;
        case '6': case '^': return KEY_6;
        case '7': case '&': return KEY_7;
        case '8': case '*': return KEY_8;
        case '9': case '(': return KEY_9;
        case '0': case ')': return KEY_0;

        case '-': case '_': return KEY_MINUS;
        case '=': case '+': return KEY_EQUALS;

        case '[': case '{': return KEY_LBRACKET;
        case ']': case '}': return KEY_RBRACKET;

        case ';': case ':': return KEY_SEMICOLON;
        case '\'': case '"': return KEY_APOSTROPHE;
        case '`': case '~': return KEY_GRAVE;
        case '\\': case '|': return KEY_BACKSLASH;

        case ',': case '<': return KEY_COMMA;
        case '.': case '>': return KEY_PERIOD;
        case '/': case '?': return KEY_SLASH;

        case ' ':  return KEY_SPACE;
        case '\t': return KEY_TAB;
        case '\n': return KEY_ENTER;

        default:
            return KEY_NONE;
    }
}

unsigned char get_scancode()
{
  unsigned char inputdata = inb(0x60);
  return inputdata;
}

unsigned char strip_key(unsigned char key)
{
  return key & 0x7F;
}

void keyboard_irq_handler(void)
{
  keyboard_handler();
  outb(0x20, 0x20);
}

void keyboard_handler()
{
  static bool e0_pending = false;
  static unsigned char e1_skip = 0;

  unsigned char scancode = get_scancode();

  if (e1_skip) {
    e1_skip--;
    return;
  }

  if (scancode == 0xE0) {
    e0_pending = true;
    return;
  }

  if (scancode == 0xE1) {
    e1_skip = 5;
    return;
  }

  bool released = scancode & 0x80;
  unsigned char raw_key = scancode & 0x7F;

  KeyCode key;
  if (e0_pending) {
    e0_pending = false;
    key = extended_scancode_to_keycode(raw_key);
  } else {
    key = scancode_to_keycode(raw_key);
  }

  if (key == KEY_NONE) { 
    return;
  }

  if (key == KEY_CAPSLOCK && !released) {
    capslock = !capslock;
  }

  if (key == KEY_SCROLLLOCK && !released) {
    scrollock = !scrollock;
  }

  if (key == KEY_NUMLOCK && !released) {
    numlock = !numlock;
  }

  keys_down[key] = !released;
  char c = keycode_to_char(key);
  if (!released && c != '\0') {
    rb_push(&g_rb, c);
  }
}

bool is_key_held(unsigned char key)
{
  KeyCode kcode = char_to_keycode(key);

  return keys_down[kcode];
}


