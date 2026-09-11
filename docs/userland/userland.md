# Userland

## global variables 
#### `static uint32_t frames[MISC_PAGE_CNT]`:
an array containing all of the misc. frames allocated to the userland. used for extra space outside of the `code_frame` and `stack_frame`

## overview
this sets up the userland frames and pages. it also calls `create_user_process()` with the `CODE_ADDR` and `STACK_ADDR + 0x1000` given. when done with userland, call `userland_teardown()`

## function analysis

### `static void map_misc_pages(void)`
map `MISC_PAGE_CNT` pages adjacent to one another, after the `STACK_ADDR`. same style as done in `userland_init()`

### `void userland_init(void)`
maps the `code_frame` and `stack_frame` using `alloc_frame()` and `map_kernel_page()`. it also calls `map_misc_pages()`, between mapping `code_frame` and `stack_frame`. also initializes the user programs, given by `userland.s`. finally calls `create_user_process()` with `CODE_ADDR` and the frame after `STACK_ADDR`

### `void userland_teardown(void)`
unmaps every frame and page mapped by `userland_init()`
