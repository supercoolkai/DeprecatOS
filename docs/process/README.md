# Process manager

## structs
### `Process` in `src/process/processQueue.h`:
represents one process, the fields of which being: 
 - `uint32_t esp`: the esp of the process 
 - `uint32_t stack_addr`:  the stack address of the process
 - `uint32_t malloc_addr`: the malloc address of the process

## overview 
contains all things to do with processes in the kernel. currently uses a round-robin scheduling algorithm

### `processQueue`
a queue which takes `Process` structs. max size of 256

### `scheduler`
controls the `processQueue` and adds/removes from it as requested. converts functions to `Process` and can add those processes to the processQueue.
