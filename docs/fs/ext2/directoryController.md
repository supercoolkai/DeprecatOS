# Directory Controller

## purpose
basically the controller of directories, whatever information is necessary from directories, this gets it. 

## global variables
#### **blk_buf:** a table which lookup_path() uses (soon to be replaced with a kmalloc)

## function analysis 
### `return_next_dir_entry()`
advances the cursor by the current entry size and returns the new entry at that position

### `ls_dir()`
iterates through a directory until it reaches the end, and returns each entry it iterates through onto a `struct dir_row` buffer

### `lookup_path()`
checks to see whether the path exists, if it does return the directory/inode at that location or if not return false
