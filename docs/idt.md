# Interrupt Descriptor Table

## data structure tables / related information

### IDT entry format

src: `src/idt/idtController.c`: `struct idt_entry`, `packed`, 8 bytes

a tabular representation of the explanation in the **structs** section:

| Off | Field | Type | Notes |
|---|---|---|---|
| 0 | `offset_low` | `uint16_t` | handler address bits 15:0 |
| 2 | `selector` | `uint16_t` | code selector, always `0x08` |
| 4 | `zero` | `uint8_t` | always 0 |
| 5 | `type_attr` | `uint8_t` | gate type + DPL + present (table below) |
| 6 | `offset_high` | `uint16_t` | handler address bits 31:16 |

### IDT gate types & layout

src: `src/idt/idtController.c`, `src/userland/syscall/syscallController.h`

`type_attr` byte decode:

| type_attr | P | DPL | gate | where |
|---|---|---|---|---|
| `0x8E` | 1 | 0 | 32-bit interrupt gate | default for every vector + IRQ/exception stubs |
| `0xEE` | 1 | 3 | 32-bit interrupt gate | syscall vector `0x80` only (DPL 3 so ring 3 can `int 0x80`) |

IDT gate vectors (occupancy map):

| vector | handler | note |
|---|---|---|
| `0x00`–`0xFF` | `default_isr_stub` | all 256 filled first (`cli; hlt`) |
| `0x21` (`FIRST_IRQ_STUB_VECTOR+1`) | `irq1_stub` | keyboard IRQ 1 |
| `0x80` (`SYSCALL_VECTOR_NUMBER`) | `syscall_stub` | `int 0x80` gate, DPL 3 |

## global variables
#### `idt_entry idt[IDT_ENTRY_AMT]`:
the global idt, use `idt_set_gate` and `idt_set_gate_type` to set an `idt_entry` inside of it.

#### `idt_ptr idtp`:
a pointer to the idt's address, loaded into the IDTR at the end of `idt_init()`

## structs

### `idt_entry` in `src/idt/idtController.c`:
describes an interrupt descriptor entry (much like `gdt_entry` in the gdt) fields being:
 - `uint16_t offset_low`: the low 16 bits of the offset
 - `uint16_t selector`: the code segment selector for the gdt
 - `uint8_t zero`: unused
 - `uint8_t type_attr`: the section which defines the idt_entry type, like interrupt gate, trap gate, etc.
 - `uint16_t offset_high`: the high 16 bits of the offset

### `idt_ptr` in `src/idt/idtController.c`:
describes an idt pointer used to announce where the IDT is. two fields:
 - `uint16_t limit`: the size of the table in bytes
 - `uint32_t base`: the address of the first entry

## overview
the controller for the idt, which handles every idt. automatically sets up the keyboard irq stub, but everything else must be set through `idt_set_gate()` and `idt_set_gate_type()`.

## function analysis

### `void idt_set_gate_type(int n, void (*handler)(void), uint8_t type)`
sets a gate at table index `n` with attribute `type` and address `(uint32_t) handler`

### `void idt_set_gate(int n, void (*handler)(void))`
calls `idt_set_gate_type()` with hardcoded `INTERUPT_GATE_TYPE` as the type


### `void idt_init(void)`
initializes the idt and does the PIC handshake right after initializing the itself and setting the idt pointer variable
