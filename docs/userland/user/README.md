# The actual userland

## overview
contains everything thats inside the userland (more precisely, everything that's outside kernel space)

### `app`
contains all apps, currently only `depsh` (shell commands) and `dinv` (vi-style text editor)

### `shell`
the userland init script basically, sets up the shell and serves as the handoff point to `app`

### `sys`
a collection of inline syscall wrappers which throw the syscall interrupt alongside the actual syscall number (sloppily made i know, maybe rewrite in the far future)
