# Timer Controller

## global variables
#### `static volatile uint32_t tick`:
the current tick (increments every 1 ms)

#### `extern void irq0_stub(void)`:
a declaration of the `irq0_stub` from `boot.s`

## overview
this sets up the `irq0_stub` in `boot.s`, as well as runs a command sequence to initialize the `PIT`

## function analysis
### `void timer_init(void)`
sets the first IDT gate to `irq0_stub` using `idt_set_gate()`, then does a command sequence with `PIT_CMD` and `PIT_CH0` which does the following (in that order):
 - sends the divisor loading command to `PIT_CMD`
 - sends the low byte of the divisor to `PIT_CH0` (using precomputed variable `uint16_t divisor`)
 - send the high byte of `divisor` to `PIT_CH0`
afterwards, it runs `sti` in asm to set the interrupt flag.

### `uint32_t timer_irq_handler(uint32_t esp)`
increments tick, closes the IRQ with `PIC1_CMD`, and schedules the given `esp`

### `uint32_t timer_get_tick(void)`
a simple getter function for `tick`
