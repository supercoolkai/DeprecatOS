# Keyboard

## global variables
#### `bool keys_down[KEY_COUNT]`:
an array representing all the keys, if a value is `true` then that key is down, else it is `false`.

#### `bool capslock`:
a seperate bool for checking if capslock is on (because its a lock key, it has different handling than `keys_down` does)

#### `bool scrollock`:
a seperate bool for checking if scrollock is on (because its a lock key, it has different handling than `keys_down` does)

#### `bool numlock`:
a seperate bool for checking if numlock is on (because its a lock key, it has different handling than `keys_down` does)

#### `ringBuffer g_rb`:
the global `ringbuffer` which is shared across everything using it.

## overview
handles the keyboard IRQ (irq1) and converts the inputted scancode into a custom enum (`KeyCode`) for easy conversion to char and back to scancode if necessary. adds everything to a `ringBuffer` (`g_rb`)

## function analysis
### `void keyboard_init(void)`
calls `rb_init()` on `g_rb` to link it to `src/util/rb/ringBuffer.c`'s `ringBuffer`.

### `KeyCode scancode_to_keycode(unsigned char scancode)`
matches a scancode to its `KeyCode`. returns a `Keycode` enum corresponding to the key.

### `KeyCode extended scancode_to_keycode(unsigned char scancode)`
like `scancode_to_keycode()`, but matches to the extended keyset.

### `char keycode_to_char(KeyCode key)`
converts a `KeyCode` to its corresponding `char`, and returns it.

### `KeyCode char_to_keycode(char c)`
the inverse of `keycode_to_char()`, converts `char` to `KeyCode`.

### `unsigned char get_scancode()`
attempts to read a scancode in the format of an `unsigned char` from port `0x60` (the primary keyboard IO port).

### `unsigned char strip_key(unsigned char key)`
throws away the press/release distinction given by the original scancode and returns the result.

### `void keyboard_irq_handler(void)`
calls `keyboard_handler()` and signals to the PIC command port (0x20) that the IRQ is closed (command 0x20)

### `void keyboard_handler`
gets the scancode, if the scancode is signaling for the extended then it notes then returns. if its 0xE1 then skip for 5 iterations. then handles the release/down case seperately, checks if the key down exists in the enum, then handles each case for it. (switches the lock if its a lock, push the key to `g_rb` if its not one **and** is not released. also updates `keys_down` to be whether or not the key is released)

### `bool is_key_held(unsigned char key)`
gets the `KeyCode` of the given `key` and returns whether or not that keycode is held using `keys_down`.
