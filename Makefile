P = `pwd`

BUNDLE_TRIM_PATHS = outs/luaposix/lua/

AR = ar rvc
RANLIB = ranlib

outs/liblua.a: lua/liblua.a
	cp $< $@

outs/lua: lua/lua
	cp $< $@

lua/liblua.a: lua/makefile
	$(MAKE) -C lua liblua.a

lua/lua: lua/makefile
	$(MAKE) -C lua lua

outs/luacfg: outs/lua lua/makefile scripts/try-update.sh
	$(MAKE) -C lua echo | scripts/try-update.sh $@

outs/luke: luke/build-aux/luke
	awk -v LUA=$P/outs/lua 'BEGIN{ f=1 } !f { print } f { f=0; print "#!" LUA }' $< > $@
	chmod +x $@

luke/build-aux/luke: outs/lua
	$(MAKE) -C luke LUA=$P/outs/lua build-aux/luke

outs/build-luaposix: outs/luke outs/luacfg outs/lua scripts/invoke-luke.lua
	outs/lua scripts/invoke-luke.lua $P outs/luacfg | xargs -0 -- sh -c 'cd luaposix && ../outs/luke "$$@"'
	touch $@

outs/luaposix/%: outs/build-luaposix outs/luke
	mkdir -p outs/luaposix/lib/ outs/luaposix/lua/
	export orig=$P; outs/lua scripts/invoke-luke.lua $P outs/luacfg | xargs -0 -- sh -c 'cd luaposix && $$orig/outs/luke install "$$@"'

# Do not edit this variable: it was generated automatically from running `lr -t
# 'type = f' outs/luaposix/ | xe -N0 -- echo` after installing luaposix with
# the previous targets.
LUAPOSIX_OUT_OFILES = outs/luaposix/lib/posix/ctype.o						\
outs/luaposix/lib/posix/dirent.o outs/luaposix/lib/posix/errno.o			\
outs/luaposix/lib/posix/fcntl.o outs/luaposix/lib/posix/fnmatch.o			\
outs/luaposix/lib/posix/glob.o outs/luaposix/lib/posix/grp.o				\
outs/luaposix/lib/posix/libgen.o outs/luaposix/lib/posix/poll.o				\
outs/luaposix/lib/posix/pwd.o outs/luaposix/lib/posix/sched.o				\
outs/luaposix/lib/posix/signal.o outs/luaposix/lib/posix/stdio.o			\
outs/luaposix/lib/posix/stdlib.o outs/luaposix/lib/posix/sys/msg.o			\
outs/luaposix/lib/posix/sys/resource.o outs/luaposix/lib/posix/sys/socket.o	\
outs/luaposix/lib/posix/sys/stat.o outs/luaposix/lib/posix/sys/statvfs.o	\
outs/luaposix/lib/posix/sys/time.o outs/luaposix/lib/posix/sys/times.o		\
outs/luaposix/lib/posix/sys/utsname.o outs/luaposix/lib/posix/sys/wait.o	\
outs/luaposix/lib/posix/syslog.o outs/luaposix/lib/posix/termio.o			\
outs/luaposix/lib/posix/time.o outs/luaposix/lib/posix/unistd.o				\
outs/luaposix/lib/posix/utime.o
LUAPOSIX_OUT_LFILES = outs/luaposix/lua/posix/_base.lua						\
outs/luaposix/lua/posix/_bitwise.lua outs/luaposix/lua/posix/_strict.lua	\
outs/luaposix/lua/posix/compat.lua outs/luaposix/lua/posix/deprecated.lua	\
outs/luaposix/lua/posix/init.lua outs/luaposix/lua/posix/sys.lua			\
outs/luaposix/lua/posix/util.lua outs/luaposix/lua/posix/version.lua

outs/install-luaposix: $(LUAPOSIX_OUT_OFILES) $(LUAPOSIX_OUT_LFILES)
	touch $@

outs/libluaposix.a: $(LUAPOSIX_OUT_OFILES)
	$(AR) $@ $^
	$(RANLIB) $@

outs/bundled_lua_modules: outs/lua bundled/make_bundled_data.lua $(LUAPOSIX_OUT_LFILES)
	outs/lua bundled/make_bundled_data.lua $@ $(BUNDLE_TRIM_PATHS) $(LUAPOSIX_OUT_LFILES)

outs/bundled_lua_modules.c: outs/bundled_lua_modules
	xxd -i $< > $@

outs/bundled_lua_modules.o: outs/bundled_lua_modules.c
	$(CC) -c $< -o $@

outs/bundle: bundled/main.c outs/liblua.a outs/libluaposix.a outs/bundled_lua_modules.o
	$(CC) -Ilua/ -DLUA_USE_READLINE bundled/main.c -Louts -llua -lluaposix -lm -ldl -lcrypt -lreadline outs/bundled_lua_modules.o -o $@
