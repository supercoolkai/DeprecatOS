# Exceptions

## global variables
#### `static const char *exc_names[32]`: a list of all the exception names 

## overview
an exception handler powered by idt gates. automatically catches errors and announces them for better debugging. alternatively custom kernel panics are able to be called as well

## function analysis

### `void panic(char *msg)`
panics with `msg` written in red on the frameBuffer. then stops the entire operating system indefinitely.

### `uint32_t exception_handler(uint32_t esp)`
the idt handler for the exceptions, announces the error in red and halts the operating system indefinitely if it is a kernel error. if it's a user error, it kills the process, warns about the error in yellow, and returns the value given by `kill_current()`
