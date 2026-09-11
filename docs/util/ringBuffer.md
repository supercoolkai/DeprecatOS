# Ring Buffer

## overview
a queue-like structure, specialized for `unsigned char`s. mainly for keyboard I/O

## function analysis
### `void rb_init(ringBuffer *rb)`
sets `rb`'s `tail` and `head` to 0.

### `bool rb_is_empty(const ringBuffer *rb)`
checks whether `tail` and `head` are equal in the given `rb`

### `bool rb_push(ringBuffer *rb, unsigned char c)`
if `rb` is 1 behind full, return `false`. otherwise, turn `queue[head]` to `c` and increment `head`

### `bool rb_pop(ringBuffer *rb, unsigned char *out)`
if the `ringBuffer` isn't already empty, outputs the character at `tail` to `out` and increments `tail`.

### `bool rb_peek(const ringBufer *rb, unsigned char *out)`
like `rb_pop()` but it does not increment `tail`.

