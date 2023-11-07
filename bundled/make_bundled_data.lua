if #arg < 2 or arg[1] == "-h" or arg[1] == "--help" or arg[1] == "-?" then
   print([[Generate the bundled data file that can be embedded on the
`bundled/main.c`.

Usage: outs/lua bundled/make_bundled_data.lua OUTPUT_FILE PREFIXES FILES...

OUTPUT_FILE is the name of the file to write. Existing data will be destroyed.

PREFIXES is a single argument that consists of a series of : separated "path
segments". Each segment will be stripped from the input files before generating
their Lua's module name.

FILES is 0 or more files that will be bundled. The name of each file is
converted to a Lua module name by removing the ".lua" or ".luac" extension (if
any). Then, all "/" are replaced by "."s. So, `outs/luaposix/lua/posix/sys.lua`
will become the module `outs.luaposix.lua.posix.sys`. But, if
`outs/luaposix/lua/` is in PREFIXES, then it will be stripped, giving
`posix.sys`.

All bundled files are precompiled as Lua.]])
   return
end

local aprefixes, prefixes = arg[2], {}
for p in string.gmatch(aprefixes, "([^:]*)") do
   prefixes[#prefixes + 1] = p
end

local files = {}
table.move(arg, 3, #arg, 1, files)
local outfname = arg[1]
print(string.format("Writing #%d files to %s", #files, outfname))

local outf <close> = assert(io.open(outfname, "wb"))
local prelude = "!1>"

local function out(fmt, ...)
   outf:write(string.pack(prelude .. fmt, ...))
end

out("T", #files)
for i = 1, #files do
   local fname = files[i]
   local content = ""
   local chunk, errmsg = loadfile(fname, "@" .. fname, "bt")
   if not chunk then
      io.stderr:write(string.format("WARNING: Could not precompile %s, inserting raw text\n", fname))
      io.stderr:write(string.format("         Reason: %s\n", errmsg))
      local handle <close> = assert(io.open(fname, "rb"))
      content = handle:read "a"
   else
      content = string.dump(chunk)
   end
   local name = string.match(fname, "^(.-)%.luac?$")
   for i = 1, #prefixes do
      local p = prefixes[i]
      if string.sub(name, 1, string.len(p)) == p then
         name = string.sub(name, string.len(p) + 1, -1)
         break
      end
   end
   local modname = string.gsub(name, "/", ".")
   print(string.format("ok %-7d %-50s => %s", string.len(content), fname, modname))
   if string.len(modname) >= 255 then
      error("module name length >= 255")
   end
   out("i1zs", string.len(modname), modname, content)
end
