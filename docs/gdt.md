# Global Descriptor Table (and TSS)

## data structure tables / related information

### selectors
src: `src/boot.s`, `src/gdt/tssController.c`

a selector is just `(index << 3) | RPL`, ring 3 selectors have `RPL=3`

| selector | GDT index | points at | used as |
|---|---|---|---|
| `0x08` | 1 | kernel code | CS ring 0 |
| `0x10` | 2 | kernel data | DS / ES / FS / GS / SS ring 0 |
| `0x18` | 3 | user code (raw) | — |
| `0x1B` | 3 | user code, RPL 3 | CS ring 3 |
| `0x20` | 4 | user data (raw) | — |
| `0x23` | 4 | user data, RPL 3 | SS ring 3 |
| `0x28` | 5 | TSS | loaded via `ltr` |

---

### TSS (tssInfo)
src: `src/gdt/tssController.h`: `tssInfo`, 104 bytes (asserted)

only ring-0 stack fields and are live/useful, all other fields are filler for hitting the 104-byte hardware layout. for each context switch `esp0` is changed to the next task's kernel stack top.

| off | field | type | notes |
|---|---|---|---|
| 0 | `link` | `uint32_t` | unused filler |
| 4 | `esp0` | `uint32_t` | ring-0 stack pointer loaded on ring3→ring0 trap |
| 8 | `ss0` | `uint32_t` | ring-0 stack segment = `0x10` |
| 12 | `filler[22]` | `uint32_t[22]` | words 3–24, unused |
| 100 | `IOBitmapOffset` | `uint32_t` | set to `104 << 16` (bitmap past TSS end = none) |

---

### GDT entry format
src: `src/gdt/tssController.c`: `struct gdt_entry`, `packed`, 8 bytes

the table below describes a GDT entry:

| off | field | type | notes |
|---|---|---|---|
| 0 | `limit` | `uint16_t` | limit bits 15:0 |
| 2 | `baseBits` | `uint16_t` | base bits 15:0 |
| 4 | `baseBitsContd` | `uint8_t` | base bits 23:16 |
| 5 | `accessByte` | `uint8_t` | present/DPL/type (table above) |
| 6 | `flags` | `uint8_t` | high nibble = granularity/size; low nibble = limit bits 19:16 |
| 7 | `baseBitsTop` | `uint8_t` | base bits 31:24 |

---

### GDT overview

src: `src/boot.s` (entries 0–4, static), `src/gdt/tssController.c` (entry 5, filled at runtime)

for each boilerplate entry filled by either `boot.s` or `tssController.c`:
 - `base` = 0
 - `limit` = `0xFFFFF`
 - 4 KiB granularity for code/data entries (1-4), byte granularity for entries 0 or 5
 - each entry accesses all 4 GiB of address space

the table below contains a clearer representation of these entries, for those who prefer tabular explanation:

| idx | raw quad | access | meaning | flags |
|---|---|---|---|---|
| 0 | `0x0000000000000000` | — | null descriptor | — |
| 1 | `0x00CF9A000000FFFF` | `0x9A` | kernel code, ring 0, exec/read | `G=1, 32-bit` |
| 2 | `0x00CF92000000FFFF` | `0x92` | kernel data, ring 0, read/write | `G=1, 32-bit` |
| 3 | `0x00CFFA000000FFFF` | `0xFA` | user code, ring 3, exec/read | `G=1, 32-bit` |
| 4 | `0x00CFF2000000FFFF` | `0xF2` | user data, ring 3, read/write | `G=1, 32-bit` |
| 5 | `0` → filled by `tss_init` | `0x89` | 32-bit TSS (available), base=`&info`, limit=103 | `G=0 (byte)` |

access byte decode:

| byte | S | DPL | S | type nibble | reads as |
|---|---|---|---|---|---|
| `0x9A` | 1 | 0 | 1 | code, readable | kernel code |
| `0x92` | 1 | 0 | 1 | data, writable | kernel data |
| `0xFA` | 1 | 3 | 1 | code, readable | user code |
| `0xF2` | 1 | 3 | 1 | data, writable | user data |
| `0x89` | 1 | 0 | 0 (system) | `1001` = 32-bit TSS avail | TSS |



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
