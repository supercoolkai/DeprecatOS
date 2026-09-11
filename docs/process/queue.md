# Process Queue

## global variables
#### `static Process *queue[256]`:
the queue containing 256 `Process`es max, like `RingBuffer`'s array.

#### `static unsigned char head`:
the "top" of the queue, one above the last `Process` placed.

#### `static unsigned char tail`:
the "bottom" of the queue, the oldest `Process` on the queue and the first to get `pop()`ed

## overview
a structure like `RingBuffer` in `util`, but its specialized for specifically `Process`, so it's put in the `process` section.

## function analysis

### `void pqueue_init(void)`
zeroes/initializes `head` and `tail`

### `bool pqueue_is_empty(void)`
checks if the queue is empty using `head == tail`. if so return `true`, else `false`

### `Process *pqueue_pop(void)`
if the queue is empty, then return `NULL`, else increment `tail` and return the `Process` at the original position.

### `Process *pqueue_peek(void)`
like `pqueue_pop()`, but it does not increment `tail`

### `bool pqueue_push(Process *p)`
if the queue is 1 behind full, return `false`. otherwise, turn `queue[head]` to `p` and increment `head`.

### `unsigned char pqueue_size(void)`
returns the difference between `head` and `tail` (the size of the queue)
