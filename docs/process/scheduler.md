# Scheduler

## global variables
#### `static Process *current`:
the current process being run, fresh off of the `processQueue`

## overview
the `Process` manager, whenever a process wants to be run it must be registered through the scheduler. 

## function analysis
### `void scheduler_init(void)`
`kmalloc`s space for `p` and initializes `p` to point to that kmalloc. sets its `malloc_addr` to be 0, and makes `current` equal to `p`. TLDR it initializes `current`.

### `uint32_t schedule(uint32_t esp)`
if the pqueue is empty, then just return `esp` because it's already going to be the next one ran. otherwise, set `current`'s `esp` to be the given `esp`, push `current` to the queue, then set current to be `pqueue_pop()`, or the oldest `Process` waiting in the queue. then, if current's `malloc_addr` is not 0, then run `tss_set_esp()` on the `malloc_addr` to set the info's `esp0` to it. then, finally return the new `esp`

### `static Process *create_proc_no_forgery(void)`
`kmalloc()`'s space for a new `Process *p`. if `kmalloc()` failed, return `NULL`. otherwise, proceed with `kmalloc()`ing `p`'s stack (passing `STACK_SIZE` as the parameter). again do the same check for if `kmalloc()` failed, if so return `NULL`. then, finally turn `p`'s `malloc_addr` to `stackAlloc`, and return `p`. does not forge a frame for the process, hence the suffix `no_forgery`

### `void create_process(void (*entry)(void))`
piggybacks off `create_proc_no_forgery()`, assigning it to `Process *p`. check if `create_proc_no_forgery()` succeeded, if not return right there. if not, set `p`'s `esp` to its `malloc_addr` + `STACK_SIZE` - 44. this is where the "fake interrupt frame", which this function forges, will be located. now, make `uint32_t *frame` a pointer to `p`'s `esp`, which will be what we write to. first zero all 8 fields of `frame`, then set: 
* `frame[8]` to be a `uint32_t` cast of `entry`
* `frame[9]` to be `0x08`
* `frame[10]` to be `0x202`
then, finally, push `p` to the queue.

### `void create_user_process(uint32_t entry, uint32_t user_stack_top)`
starts off like `create_process()` does, by calling `create_proc_no_forgery()` and assigning it to `Process *p`. check if it succeeded, if not return. then, do the same edit to `p`'s `esp` as `create_process()` does, and initialize the forged frame the same way as `create_process()`. the `frame` forgery is where this function differs from `create_process()`, instead of the default forgery it does:
* set `frame[8]` to `entry`, no cast since its already `uint32_t`
* set `frame[9]` to be `0x1B`
* set `frame[10]` to be `0x202`
* set `frame[11]` to be `user_stack_top`
* set `frame[12]` to be `0x23`
then, finally push `p` to the queue.

### `uint32_t kill_current(void)`
first, warns to the framebuffer and serial that it's attempting the pkill. then, check if the queue is empty. if so (which is HIGHLY unlikely by the way, so you will most likely never encounter this error message), warn that an "EPIC PKILL FAIL" happened and `kfree()` `0x00000000`, as well as returning `0xFFFFFFFF`. If it's not empty, save `current` to a temporary variable (`victim`)  save `pqueue_pop()` to a variable (`next`) and set `current` to `next`. also, if the new `current`'s `malloc_addr` has been properly setup, set the new `esp0` to `malloc_addr` + `STACK_SIZE`. finally, call `userland_teardown()`, `kfree()` the victim's stack address (`malloc_addr`) as well as `victim` itself, and return the new `current`'s `esp`. 
