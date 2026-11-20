# port IO utility

## overview
a utility which allows direct communication with ports without having to use assembly or inline every time

## function analysis

### `static inline void outl(uint16_t port, uint32_t val)`
sends `val` to `port` using inline assembly


### `static inline void outb(uint16_t port, uint8_t val)`
sends `val` to `port` using inline assembly

### `static inline void outw(uint16_t port, uint16_t val)`
sends `val` to `port` using inline assembly

### `static inline uint8_t inb(uint16_t port)`
reads a value from `port` using inline assembly

### `static inline uint32_t inl(uint16_t port)`
reads a value from `port` using inline assembly

### `static inline uint16_t inw(uint16_t port)`
reads a value from `port` using inline assembly
