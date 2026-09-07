# Global Descriptor Table 

## global variables
#### `tssInfo info`:
the global information for the tss. can be modified using `tss_set_esp()`

## structs

### `tssInfo` in `src/gdt/tssController.h`
information for task state segments (tss), fields being: 
 - `uint32_t link`: filler
 - `uint32_t esp0`: the stack pointers used to load the stack when a privilege change happens from lower->higher levels
 - `uint32_t ss0`: the segment selectors used to load the stack in the same situation as the `esp0`
 - `uint32_t filler[22]`: words 3 -> 24 being reserved inclusive
 - `uint32_t IOBitmapOffset`: the offset to the entry in the IOPB

### `gdt_entry` in `src/gdt/tssController.c`:
an entry in the global descriptor table, containing fields:
 - `uint16_t limit`: tells the maximum addressable unit 
 - `uint16_t baseBits`: the linear address of where the segment begins
 - `uint8_t baseBitsContd`: the next 8 bits of `baseBits`
 - `uint8_t accessByte`: contains a few bits representing the information of the entry (for example whether it is executable, readable/writeable, present, etc)
 - `uint8_t flags`: contains flags, being granularity, size, and long-mode code flag
 - `uint8_t baseBitsTop`: the last 8 bits of `baseBits`

## function analysis

### `void tss_init(void)`
sets up the global variable `info` by pointing its `esp0` to the given `stack_top` from `boot.s`, and setting its `ss0` to 0x10 (kernel data segment index on the stack) . also sets up its `IOBitmapOffset` to 104 << 16 (1 past the segment limit, shifted up 16 bits to fit the 16-bit field (bitmap field in hardware is split into 16-bit fields).
also sets the gdt[5] (the placeholder slot). finally `ltr` with `sel` = 0x28 (the selector for the gdt[5])

### `void tss_set_esp(uint32_t n)`
sets the info's `esp0` to `n`
