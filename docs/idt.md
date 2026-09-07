# Interrupt Descriptor Table

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
