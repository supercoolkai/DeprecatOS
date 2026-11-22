# App controller

## app struct format
src `user/app/appCtl.h`

| offset | field | type |
| --- | --- | --- |
| 0 | `name` | `char *` |
| 4 | `fn` | `void (*)(char *args)` |

## overview
the app manager, everything (properly wired) is runnable from the shell

## apps
### `depsh`:
commands for the shell, general purpose like `cat`, `touch`, `mkdir`, `rm`, etc. see `docs/userland/user/app/depsh.md` for more info

### `dinv`:
a vi-like modal text editor (short for Dinv-Is-Not-Vi). btw NO arrow key support, intended for maximum larp, ts not nano smh. see `docs/userland/user/app/dinv.md` for more info
