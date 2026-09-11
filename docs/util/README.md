# Util

## structs
### `ringBuffer` in `src/util/rb/ringBuffer.h`:
a struct representing a ring buffer, used mainly in `ringBuffer`, the fields being:
 - `volatile unsigned char queue[256]`: the queue of 256 `unsigned char`s 
 - `volatile unsigned char head`: the "head" of the `ringBuffer`, the number of elements placed, looping from 0-255
 - `volatile unsigned char tail`: the "tail" of the `ringBuffer`, the number of elements removed, looping from 0-255. always supposed to be less than or equal to `head`.

## overview 
a group of utilities which cannot be categorized under one directory. essentially a misc directory for tools

### `ringBuffer`
a queue-like structure which contains chars (typically used for keyboard IO)

### `hexPrinter`
a general utility to print `uint32_t` values in a given color. 
