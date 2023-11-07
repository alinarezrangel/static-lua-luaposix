# Partially-static build of Lua 5.4 and LuaPosix #

This repo contains code to build an unpatched Lua 5.4 interpreter, an
non-dynamically-linked unpatched LuaPosix and to build a custom Lua interpreter
that automatically loads said "almost-static" LuaPosix while acting just like
upstream's `lua` program.

## Build ##

Run `make all`. If you want to configure anything (CFLAGS, the compiler used,
etc) you will have to customize several files:

 1. Lua's `lua/makefile`.
 2. LuaPosix invocation of `luke`: `scripts/invoke-luke.lua`.
 3. This project's own `Makefile`.

In the future, it would be nice to have all projects read the same variables
from a single file. Failing to change any of the mentioned files could result
in weird compilation errors (this is particularly true if significant changes
are made: for example, enabling ASan on the Lua interpreter while not enabling
it on the `outs/bundle` executable *will* result in linking errors).

After building, the `outs/` directory will contain (amongst other things) a
`bundle` executable. This executable should only depend on your normal system's
libraries, **not** on luaposix nor Lua 5.4 (you can validate that using
`ldd`(1)).

While it should be ABI compatible with any Lua 5.4 library, I still recommend
you to change the `Makefile` so that Lua 5.4 is dynamically linked in the case
you plan to load a lot of C extensions with it (that way you can be sure it
will work).

## Usage ##

```
./outs/bundle
./outs/bundle -luni=posix.unistd -e 'print(uni.isatty(uni.STDOUT_FILENO))'
# Etc...
```

The `outs/bundle` program should behave identically to the `lua` program. This
includes command line parsing, readed environment variables, the REPL, etc.

You can deploy this program (with it's few dynamic dependencies) to any other
system, without needing to install LuaPosix or Lua there.
