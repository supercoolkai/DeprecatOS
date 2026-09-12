# Process Queue

## data structure tables / related information

### process
src: `src/process/queue/processQueue.h`: `Process`, 12 bytes

a table of the struct:

| off | field | type | notes |
|---|---|---|---|
| 0 | `esp` | `uint32_t` | saved stack pointer (into the forged/live frame) |
| 4 | `stack_addr` | `uint32_t` | present in struct; scheduler drives off `malloc_addr` |
| 8 | `malloc_addr` | `uint32_t` | base of the `kmalloc`'d `STACK_SIZE` stack; `0` = the bootstrap `current` with no stack |

`STACK_SIZE = 1024` (`scheduler.h`). kernel stack top = `malloc_addr + STACK_SIZE`; TSS `esp0` is set there on switch-in. Ready queue is a 256-slot ring (`pqueue_*`).

---

## global variables
#### `static Process *queue[256]`:
the queue containing 256 `Process`es max, like `RingBuffer`'s array.

#### `static unsigned char head`:
the "top" of the queue, the last `Process` placed.

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
