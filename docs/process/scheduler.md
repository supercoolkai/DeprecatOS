# Scheduler

## data structure tables / related information

### "forged" task stack frame

src: `src/process/scheduler/scheduler.c` (`create_process` / `create_user_process`), consumed by `src/boot.s` `irq0_stub`

for each new brand new task, there must be an interrupt stack. so, to force the system to accept it, we "forge" a stack to look exactly like one from an interrupt, so that the ASM switch code's `popa; iret` "resumes" it into `entry`. let `frame[]` be our forged stack frame. `p->esp` points at `frame[0]`. there are two types of these frames:
 - kernel frame: 44 bytes (11 dwords)
 - user frame: 52 bytes (13 dwords: `iret` to ring 3 also pops `esp` and `ss`)

order is fixed by `irq0_stub`: `pusha` (8 regs) then the CPU's iret frame. `pusha` stores high->low, so in memory low->high it's EDI...EAX

a table for the forged frame's layout:

| idx | off | slot | new-kernel value | new-user value |
|---|---|---|---|---|
| 0 | 0 | EDI | 0 | 0 |
| 1 | 4 | ESI | 0 | 0 |
| 2 | 8 | EBP | 0 | 0 |
| 3 | 12 | ESP (ignored by `popa`) | 0 | 0 |
| 4 | 16 | EBX | 0 | 0 |
| 5 | 20 | EDX | 0 | 0 |
| 6 | 24 | ECX | 0 | 0 |
| 7 | 28 | EAX | 0 | 0 |
| 8 | 32 | EIP | `entry` | `entry` |
| 9 | 36 | CS | `0x08` | `0x1B` |
| 10 | 40 | EFLAGS | `0x202` (IF set) | `0x202` (IF set) |
| 11 | 44 | ESP | — | `user_stack_top` |
| 12 | 48 | SS | — | `0x23` |

> the `44` / `52` sizes and slot indices are hardcoded to the exact
> push order in `boot.s`. if you change a push there, both must change.
> will be fixed soon, hang tight!!
---

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
