# Timer Controller

## global variables
#### `static volatile uint32_t tick`:
the current tick (increments every 1 ms)

#### `static uint32_t startup_epoch`:
the epoch found at startup and init

#### `extern void irq0_stub(void)`:
a declaration of the `irq0_stub` from `boot.s`

## overview
this sets up the `irq0_stub` in `boot.s`, as well as runs a command sequence to initialize the `PIT`

## function analysis

### `static uint8_t cmos_read(uint8_t reg)`
sends the given register to `RTC_COMMAND_PORT`, then returns the given value from `RTC_READ_PORT`

### `static uint8_t bin_to_dec(uint8_t bin)`
converts a binary `uint8_t` to decimal form, util function. 
NOTE: whenever another caller requires it move it to `etc/lib`

### `static uint32_t get_year_startup(void)`
gets the current year. guesses which century it is, denies all centuries above 21 or below 20 and sets it to the closest guess. finally returns the calculated year.

### `static void get_seconds_startup(void)`
first waits for the current status to not be updating. then gets all relevant values, including `year`, `month`, `day`, `hour`, etc. also accounts for if the clock is 12h instead of 24h. then finally plugs in these values to a formula which gets the epoch, and sets `startup_epoch` to it.

### `void timer_init(void)`
sets the first IDT gate to `irq0_stub` using `idt_set_gate()`, then does a command sequence with `PIT_CMD` and `PIT_CH0` which does the following (in that order):
 - sends the divisor loading command to `PIT_CMD`
 - sends the low byte of the divisor to `PIT_CH0` (using precomputed variable `uint16_t divisor`)
 - send the high byte of `divisor` to `PIT_CH0`
afterwards, it runs `sti` in asm to set the interrupt flag.
Also calls `get_seconds_startup()` to setup `startup_epoch`

### `uint32_t timer_irq_handler(uint32_t esp)`
increments tick, closes the IRQ with `PIC1_CMD`, and schedules the given `esp`

### `uint32_t timer_get_tick(void)`
a simple getter function for `tick`

### `uint32_t timer_get_epoch(void)`
returns the current epoch (calculated from `startup_epoch` and `tick`) in milliseconds

### `uint32_t timer_get_epoch_sec(void)`
returns the current epoch in seconds.
