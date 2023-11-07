local P, infile = arg[1], arg[2]

-- Read lua/makefile vars:
local vars = {}
for line in io.lines(infile) do
   local var, val = string.match(line, "^([a-zA-Z0-9]+)%s*=%s*(.*)$")
   if var and val then
      vars[var] = val
   end
end

-- Write luke's vars:
local function set(lukevar, value)
   io.write(lukevar .. "=" .. value .. "\0")
end

local function mk(var, def)
   return vars[var] or def or ""
end

set("package", "bundled_luaposix")
set("version", "bundled-1")
set("PREFIX", os.getenv"PREFIX" or P)
set("LUA", P .. "/outs/lua")
set("LUA_INCDIR", P .. "/lua")
local CC = mk("CC", os.getenv"CC")
set("CC", CC)
set("LD", mk("LD", os.getenv"LD" or CC))
set("CFLAGS", mk"CFLAGS" .. " " .. mk"MYCFLAGS")
set("LDFLAGS", mk"LDFLAGS" .. " " .. mk"MYLDFLAGS")
set("LIBFLAG", "-c")
set("LIB_EXTENSION", "o")
set("LIBS", mk"LIBS" .. " " .. mk"MYLIBS")
set("INST_LIBDIR", P .. "/outs/luaposix/lib")
set("INST_LUADIR", P .. "/outs/luaposix/lua")
