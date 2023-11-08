#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <signal.h>

#include "luaconf.h"
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"


#define BUNDLED_COPYRIGHT "bundled-lua Copyright (C) 2023 Alejandro Linarez Rangel"
#define LUAPOSIX_COPYRIGHT "luaposix Copyright (C) 2006-2023 luaposix authors"


extern int luaopen_posix_ctype(lua_State* L);
extern int luaopen_posix_dirent(lua_State* L);
extern int luaopen_posix_errno(lua_State* L);
extern int luaopen_posix_fcntl(lua_State* L);
extern int luaopen_posix_fnmatch(lua_State* L);
extern int luaopen_posix_glob(lua_State* L);
extern int luaopen_posix_grp(lua_State* L);
extern int luaopen_posix_libgen(lua_State* L);
extern int luaopen_posix_poll(lua_State* L);
extern int luaopen_posix_pwd(lua_State* L);
extern int luaopen_posix_sched(lua_State* L);
extern int luaopen_posix_signal(lua_State* L);
extern int luaopen_posix_stdio(lua_State* L);
extern int luaopen_posix_stdlib(lua_State* L);
extern int luaopen_posix_sys_msg(lua_State* L);
extern int luaopen_posix_sys_resource(lua_State* L);
extern int luaopen_posix_sys_socket(lua_State* L);
extern int luaopen_posix_sys_stat(lua_State* L);
extern int luaopen_posix_sys_statvfs(lua_State* L);
extern int luaopen_posix_sys_time(lua_State* L);
extern int luaopen_posix_sys_times(lua_State* L);
extern int luaopen_posix_sys_utsname(lua_State* L);
extern int luaopen_posix_sys_wait(lua_State* L);
extern int luaopen_posix_syslog(lua_State* L);
extern int luaopen_posix_termio(lua_State* L);
extern int luaopen_posix_time(lua_State* L);
extern int luaopen_posix_unistd(lua_State* L);
extern int luaopen_posix_utime(lua_State* L);

struct module
{
    lua_CFunction opener;
    const char* name;
};

#define ARRLEN(arr) (sizeof(arr) / sizeof((arr)[0]))

static struct module posix_modules[] = {
    {&luaopen_posix_ctype,        "posix.ctype"},
    {&luaopen_posix_dirent,       "posix.dirent"},
    {&luaopen_posix_errno,        "posix.errno"},
    {&luaopen_posix_fcntl,        "posix.fcntl"},
    {&luaopen_posix_fnmatch,      "posix.fnmatch"},
    {&luaopen_posix_glob,         "posix.glob"},
    {&luaopen_posix_grp,          "posix.grp"},
    {&luaopen_posix_libgen,       "posix.libgen"},
    {&luaopen_posix_poll,         "posix.poll"},
    {&luaopen_posix_pwd,          "posix.pwd"},
    {&luaopen_posix_sched,        "posix.sched"},
    {&luaopen_posix_signal,       "posix.signal"},
    {&luaopen_posix_stdio,        "posix.stdio"},
    {&luaopen_posix_stdlib,       "posix.stdlib"},
    {&luaopen_posix_sys_msg,      "posix.sys.msg"},
    {&luaopen_posix_sys_resource, "posix.sys.resource"},
    {&luaopen_posix_sys_socket,   "posix.sys.socket"},
    {&luaopen_posix_sys_stat,     "posix.sys.stat"},
    {&luaopen_posix_sys_statvfs,  "posix.sys.statvfs"},
    {&luaopen_posix_sys_time,     "posix.sys.time"},
    {&luaopen_posix_sys_times,    "posix.sys.times"},
    {&luaopen_posix_sys_utsname,  "posix.sys.utsname"},
    {&luaopen_posix_sys_wait,     "posix.sys.wait"},
    {&luaopen_posix_syslog,       "posix.syslog"},
    {&luaopen_posix_termio,       "posix.termio"},
    {&luaopen_posix_time,         "posix.time"},
    {&luaopen_posix_unistd,       "posix.unistd"},
    {&luaopen_posix_utime,        "posix.utime"},
    {NULL, NULL},
};

extern char outs_bundled_lua_modules[];
extern int outs_bundled_lua_modules_len;

struct index_field
{
    char* name;
    size_t name_len;
    char* content;
    size_t content_len;
};

static struct index_field* bundled_index;
static size_t bundled_index_len;

static char* read_size_t(char* p, size_t* res)
{
    *res = 0;
    for(size_t i = 0; i < sizeof(size_t); i++)
        *res = (*res << 8) | (unsigned char) (*p++);
    return p;
}

static int make_index(void)
{
    read_size_t(outs_bundled_lua_modules, &bundled_index_len);
    bundled_index = calloc(sizeof(struct index_field), bundled_index_len);
    if(!bundled_index)
        return 0;
    char* p = outs_bundled_lua_modules + sizeof(size_t);
    for(size_t i = 0; i < bundled_index_len; i++)
    {
        size_t name_len = (size_t) *p++;
        char* name = p;
        p += name_len + 1; // +1 for the null char
        size_t content_len = 0;
        p = read_size_t(p, &content_len);
        char* content = p;
        p += content_len;
        bundled_index[i].name = name;
        bundled_index[i].name_len = name_len;
        bundled_index[i].content = content;
        bundled_index[i].content_len = content_len;
    }
    return 1;
}

static void adjust_arguments(lua_State* L, int nparams)
{
    int nargs = lua_gettop(L);
    if(nargs < nparams)
    {
        for(int i = nargs; i < nparams; i++)
            lua_pushnil(L);
    }
    else if(nargs > nparams)
    {
        lua_pop(L, nargs - nparams);
    }
}

static int precompiled_loader(lua_State* L)
{
    // Called with 2 arguments: the module name and extra data (that we don't
    // care about)
    adjust_arguments(L, 2);

    lua_getfield(L, LUA_REGISTRYINDEX, "bundled_cmodules");
    // [modname, edata, bundled]
    lua_pushnil(L);
    // [modname, edata, bundled, nil]
    lua_copy(L, -4, -1);
    // [modname, edata, bundled, modname]
    lua_gettable(L, -2);
    // [modname, edata, bundled, modchunk]
    lua_insert(L, -4);
    // [modchunk, modname, edata, bundled]
    lua_pop(L, 3);
    // [modchunk]
    lua_call(L, 0, 1);
    // [res]
    return 1;
}

static int bundled_loader(lua_State* L)
{
    adjust_arguments(L, 2);

    // [modname, edata]
    lua_pop(L, 1);
    // [modname]
    size_t name_len;
    const char* name = lua_tolstring(L, -1, &name_len);
    for(size_t i = 0; i < bundled_index_len; i++)
    {
        struct index_field f = bundled_index[i];
        if(f.name_len != name_len)
            continue;
        if(memcmp(f.name, name, f.name_len) == 0)
        {
            char chunkname[257];
            snprintf(chunkname, sizeof(chunkname), "=%s", f.name);
            chunkname[256] = 0;
            // [modname]
            lua_pop(L, 1);
            // []
            int status = luaL_loadbuffer(L, f.content, f.content_len, chunkname);
            // [modchunk]
            if(status == LUA_OK)
            {
                lua_call(L, 0, 1);
                // [res]
                return 1;
            }
            else
            {
                lua_pushfstring(L, "ERROR %d\n", status);
                lua_error(L);
                return 0;
            }
        }
    }
    // [modname]
    lua_pop(L, 1);
    return 0;
}

static int preload_init_loader(lua_State* L)
{
    // This preloader will try ".init"-less versions of already preloaded
    // modules. So, if `posix.init` is preloaded and `posix` is required, it
    // will find `posix.init`.
    adjust_arguments(L, 2);

    // [modname, edata]
    lua_pop(L, 1);
    // [modname]
    lua_pushliteral(L, ".init");
    // [modname, ".init"]
    lua_concat(L, 2);
    // [modname]    -- Now modname has a trailing ".init"
    lua_getfield(L, LUA_REGISTRYINDEX, LUA_PRELOAD_TABLE);
    // [modname, packages.preload]
    lua_insert(L, -2);
    // [packages.preload, modname]
    lua_pushnil(L);
    // [packages.preload, modname, nil]
    lua_copy(L, -2, -1);
    // [packages.preload, modname, modname]
    lua_insert(L, -3);
    // [modname, packages.preload, modname]
    lua_gettable(L, -2);
    // [modname, packages.preload, loader]
    lua_insert(L, -3);
    // [loader, modname, packages.preload]
    lua_pop(L, 1);
    // [loader, modname]
    lua_pushnil(L);
    // [loader, modname, nil]
    lua_call(L, 2, 1);
    // [res]
    return 1;
}

static void fill_state_with_luaposix(lua_State* L)
{
    lua_createtable(L, 0, ARRLEN(posix_modules));
    for(struct module* mod = posix_modules; mod->name && mod->opener; mod++)
    {
        lua_pushstring(L, mod->name);
        lua_pushcfunction(L, mod->opener);
        lua_settable(L, -3);
    }
    lua_setfield(L, LUA_REGISTRYINDEX, "bundled_cmodules");

    lua_getfield(L, LUA_REGISTRYINDEX, LUA_PRELOAD_TABLE);
    for(struct module* mod = posix_modules; mod->name && mod->opener; mod++)
    {
        lua_pushstring(L, mod->name);
        lua_pushcfunction(L, &precompiled_loader);
        lua_settable(L, -3);
    }
    lua_pop(L, 1);
}

static void fill_state_with_bundled(lua_State* L)
{
    lua_getfield(L, LUA_REGISTRYINDEX, LUA_PRELOAD_TABLE);
    for(size_t i = 0; i < bundled_index_len; i++)
    {
        struct index_field f = bundled_index[i];
        lua_pushstring(L, f.name);
        lua_pushcfunction(L, &bundled_loader);
        lua_settable(L, -3);
    }
    lua_pop(L, 1);
}

static void fill_state_with_initless(lua_State* L)
{
    lua_newtable(L);
    lua_getfield(L, LUA_REGISTRYINDEX, LUA_PRELOAD_TABLE);
    // [acc, package.preload]
    lua_pushnil(L);
    while(lua_next(L, -2) != 0)
    {
        // [acc, package.preload, key, value]
        lua_pop(L, 1);
        // [acc, package.preload, key]
        if(lua_type(L, -1) == LUA_TSTRING)
        {
            // [acc, package.preload, modname]
            size_t len = 0;
            const char* modname = lua_tolstring(L, -1, &len);
            if(len >= 5 && strcmp(modname + len - 5, ".init") == 0)
            {
                // Ends with ".init", add init-less version:
                lua_pushlstring(L, modname, len - 5);
                // [acc, package.preload, modname, newname]
                lua_pushcfunction(L, &preload_init_loader);
                // [acc, package.preload, modname, newname, newloader]
                lua_settable(L, -5);
                // [acc, package.preload, modname]
            }
        }
    }

    // [acc, package.preload]
    lua_pushnil(L);
    while(lua_next(L, -3) != 0)
    {
        // [acc, package.preload, key, value]
        lua_pushnil(L);
        // [acc, package.preload, key, value, nil]
        lua_copy(L, -3, -1);
        // [acc, package.preload, key, value, key]
        lua_insert(L, -2);
        // [acc, package.preload, key, key, value]
        lua_settable(L, -4);
        // [acc, package.preload, key]
    }

    // [acc]
    lua_pop(L, 1);
}

// The mayority of the following program was taken almost verbatim from
// `lua/lua.c`, which is Lua stand-alone interpreter. The reason for copying it
// rather than reimplementing it is that I decided that I wanted the same
// behaviour than the Lua interpreter by default (including REPL and
// environment variables).
//
// When bundling an entrypoint, all of the normal behaviour (except for the
// REPL and command-line parsing) will be left as-is. If the developer wants to
// disable all of this they can do it with the macros documented below.

#if !defined(BUNDLED_PROGNAME)
#define BUNDLED_PROGNAME "lua"
#endif

#if !defined(LUA_INIT_VAR)
#define LUA_INIT_VAR "LUA_INIT"
#endif

#define LUA_INITVARVERSION	LUA_INIT_VAR LUA_VERSUFFIX

static const char* progname = BUNDLED_PROGNAME;

static lua_State *globalL = NULL;

/*
** Use 'sigaction' when available.
*/
static void setsignal (int sig, void (*handler)(int)) {
    struct sigaction sa;
    sa.sa_handler = handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);  /* do not mask any signal */
    sigaction(sig, &sa, NULL);
}

/*
** Hook set by signal function to stop the interpreter.
*/
static void lstop (lua_State *L, lua_Debug *ar) {
    (void)ar;  /* unused arg. */
    lua_sethook(L, NULL, 0, 0);  /* reset hook */
    luaL_error(L, "interrupted!");
}

/*
** Function to be called at a C signal. Because a C signal cannot
** just change a Lua state (as there is no proper synchronization),
** this function only sets a hook that, when called, will stop the
** interpreter.
*/
static void laction (int i) {
    int flag = LUA_MASKCALL | LUA_MASKRET | LUA_MASKLINE | LUA_MASKCOUNT;
    setsignal(i, SIG_DFL); /* if another SIGINT happens, terminate process */
    lua_sethook(globalL, lstop, flag, 1);
}

static void print_usage (const char *badoption) {
    lua_writestringerror("%s: ", progname);
    if (badoption[1] == 'e' || badoption[1] == 'l')
        lua_writestringerror("'%s' needs argument\n", badoption);
    else
        lua_writestringerror("unrecognized option '%s'\n", badoption);
    lua_writestringerror(
        "usage: %s [options] [script [args]]\n"
        "Available options are:\n"
        "  -e stat   execute string 'stat'\n"
        "  -i        enter interactive mode after executing 'script'\n"
        "  -l mod    require library 'mod' into global 'mod'\n"
        "  -l g=mod  require library 'mod' into global 'g'\n"
        "  -v        show version information\n"
        "  -E        ignore environment variables\n"
        "  -W        turn warnings on\n"
        "  --        stop handling options\n"
        "  -         stop handling options and execute stdin\n"
        ,
        progname);
}

/*
** Prints an error message, adding the program name in front of it
** (if present)
*/
static void l_message (const char *pname, const char *msg) {
    if (pname) lua_writestringerror("%s: ", pname);
    lua_writestringerror("%s\n", msg);
}

/*
** Check whether 'status' is not OK and, if so, prints the error
** message on the top of the stack. It assumes that the error object
** is a string, as it was either generated by Lua or by 'msghandler'.
*/
static int report (lua_State *L, int status) {
    if (status != LUA_OK) {
        const char *msg = lua_tostring(L, -1);
        l_message(progname, msg);
        lua_pop(L, 1);  /* remove message */
    }
    return status;
}

/*
** Message handler used to run all chunks
*/
static int msghandler (lua_State *L) {
    const char *msg = lua_tostring(L, 1);
    if (msg == NULL) {  /* is error object not a string? */
        if (luaL_callmeta(L, 1, "__tostring") &&  /* does it have a metamethod */
            lua_type(L, -1) == LUA_TSTRING)  /* that produces a string? */
            return 1;  /* that is the message */
        else
            msg = lua_pushfstring(L, "(error object is a %s value)",
                                  luaL_typename(L, 1));
    }
    luaL_traceback(L, L, msg, 1);  /* append a standard traceback */
    return 1;  /* return the traceback */
}

/*
** Interface to 'lua_pcall', which sets appropriate message function
** and C-signal handler. Used to run all chunks.
*/
static int docall (lua_State *L, int narg, int nres) {
    int status;
    int base = lua_gettop(L) - narg;  /* function index */
    lua_pushcfunction(L, msghandler);  /* push message handler */
    lua_insert(L, base);  /* put it under function and args */
    globalL = L;  /* to be available to 'laction' */
    setsignal(SIGINT, laction);  /* set C-signal handler */
    status = lua_pcall(L, narg, nres, base);
    setsignal(SIGINT, SIG_DFL); /* reset C-signal handler */
    lua_remove(L, base);  /* remove message handler from the stack */
    return status;
}

static void print_version (void) {
    lua_writestring(LUA_COPYRIGHT, strlen(LUA_COPYRIGHT));
    lua_writeline();
    lua_writestring(BUNDLED_COPYRIGHT, strlen(BUNDLED_COPYRIGHT));
    lua_writeline();
    lua_writestring(LUAPOSIX_COPYRIGHT, strlen(LUAPOSIX_COPYRIGHT));
    lua_writeline();
}

/*
** Create the 'arg' table, which stores all arguments from the
** command line ('argv'). It should be aligned so that, at index 0,
** it has 'argv[script]', which is the script name. The arguments
** to the script (everything after 'script') go to positive indices;
** other arguments (before the script name) go to negative indices.
** If there is no script name, assume interpreter's name as base.
** (If there is no interpreter's name either, 'script' is -1, so
** table sizes are zero.)
*/
static void createargtable (lua_State *L, char **argv, int argc, int script) {
    int i, narg;
    narg = argc - (script + 1);  /* number of positive indices */
    lua_createtable(L, narg, script + 1);
    for (i = 0; i < argc; i++) {
        lua_pushstring(L, argv[i]);
        lua_rawseti(L, -2, i - script);
    }
    lua_setglobal(L, "arg");
}

static int dochunk (lua_State *L, int status) {
    if (status == LUA_OK) status = docall(L, 0, 0);
    return report(L, status);
}

static int dofile (lua_State *L, const char *name) {
    return dochunk(L, luaL_loadfile(L, name));
}

static int dostring (lua_State *L, const char *s, const char *name) {
    return dochunk(L, luaL_loadbuffer(L, s, strlen(s), name));
}

/*
** Receives 'globname[=modname]' and runs 'globname = require(modname)'.
** If there is no explicit modname and globname contains a '-', cut
** the sufix after '-' (the "version") to make the global name.
*/
static int dolibrary (lua_State *L, char *globname) {
    int status;
    char *suffix = NULL;
    char *modname = strchr(globname, '=');
    if (modname == NULL) {  /* no explicit name? */
        modname = globname;  /* module name is equal to global name */
        suffix = strchr(modname, *LUA_IGMARK);  /* look for a suffix mark */
    }
    else {
        *modname = '\0';  /* global name ends here */
        modname++;  /* module name starts after the '=' */
    }
    lua_getglobal(L, "require");
    lua_pushstring(L, modname);
    status = docall(L, 1, 1);  /* call 'require(modname)' */
    if (status == LUA_OK) {
        if (suffix != NULL)  /* is there a suffix mark? */
            *suffix = '\0';  /* remove sufix from global name */
        lua_setglobal(L, globname);  /* globname = require(modname) */
    }
    return report(L, status);
}

/*
** Push on the stack the contents of table 'arg' from 1 to #arg
*/
static int pushargs (lua_State *L) {
    int i, n;
    if (lua_getglobal(L, "arg") != LUA_TTABLE)
        luaL_error(L, "'arg' is not a table");
    n = (int)luaL_len(L, -1);
    luaL_checkstack(L, n + 3, "too many arguments to script");
    for (i = 1; i <= n; i++)
        lua_rawgeti(L, -i, i);
    lua_remove(L, -i);  /* remove table from the stack */
    return n;
}

static int handle_script (lua_State *L, char **argv) {
    int status;
    const char *fname = argv[0];
    if (strcmp(fname, "-") == 0 && strcmp(argv[-1], "--") != 0)
        fname = NULL;  /* stdin */
    status = luaL_loadfile(L, fname);
    if (status == LUA_OK) {
        int n = pushargs(L);  /* push arguments to script */
        status = docall(L, n, LUA_MULTRET);
    }
    return report(L, status);
}

/* bits of various argument indicators in 'args' */
#define has_error	1	/* bad option */
#define has_i		2	/* -i */
#define has_v		4	/* -v */
#define has_e		8	/* -e */
#define has_E		16	/* -E */

/*
** Traverses all arguments from 'argv', returning a mask with those
** needed before running any Lua code or an error code if it finds any
** invalid argument. In case of error, 'first' is the index of the bad
** argument.  Otherwise, 'first' is -1 if there is no program name,
** 0 if there is no script name, or the index of the script name.
*/
static int collectargs (char **argv, int *first) {
    int args = 0;
    int i;
    if (argv[0] != NULL) {  /* is there a program name? */
        if (argv[0][0])  /* not empty? */
            progname = argv[0];  /* save it */
    }
    else {  /* no program name */
        *first = -1;
        return 0;
    }
    for (i = 1; argv[i] != NULL; i++) {  /* handle arguments */
        *first = i;
        if (argv[i][0] != '-')  /* not an option? */
            return args;  /* stop handling options */
        switch (argv[i][1]) {  /* else check option */
        case '-':  /* '--' */
            if (argv[i][2] != '\0')  /* extra characters after '--'? */
                return has_error;  /* invalid option */
            *first = i + 1;
            return args;
        case '\0':  /* '-' */
            return args;  /* script "name" is '-' */
        case 'E':
            if (argv[i][2] != '\0')  /* extra characters? */
                return has_error;  /* invalid option */
            args |= has_E;
            break;
        case 'W':
            if (argv[i][2] != '\0')  /* extra characters? */
                return has_error;  /* invalid option */
            break;
        case 'i':
            args |= has_i;  /* (-i implies -v) *//* FALLTHROUGH */
        case 'v':
            if (argv[i][2] != '\0')  /* extra characters? */
                return has_error;  /* invalid option */
            args |= has_v;
            break;
        case 'e':
            args |= has_e;  /* FALLTHROUGH */
        case 'l':  /* both options need an argument */
            if (argv[i][2] == '\0') {  /* no concatenated argument? */
                i++;  /* try next 'argv' */
                if (argv[i] == NULL || argv[i][0] == '-')
                    return has_error;  /* no next argument or it is another option */
            }
            break;
        default:  /* invalid option */
            return has_error;
        }
    }
    *first = 0;  /* no script name */
    return args;
}

/*
** Processes options 'e' and 'l', which involve running Lua code, and
** 'W', which also affects the state.
** Returns 0 if some code raises an error.
*/
static int runargs (lua_State *L, char **argv, int n) {
    int i;
    for (i = 1; i < n; i++) {
        int option = argv[i][1];
        lua_assert(argv[i][0] == '-');  /* already checked */
        switch (option) {
        case 'e':  case 'l': {
            int status;
            char *extra = argv[i] + 2;  /* both options need an argument */
            if (*extra == '\0') extra = argv[++i];
            lua_assert(extra != NULL);
            status = (option == 'e')
                ? dostring(L, extra, "=(command line)")
                : dolibrary(L, extra);
            if (status != LUA_OK) return 0;
            break;
        }
        case 'W':
            lua_warning(L, "@on", 0);  /* warnings on */
            break;
        }
    }
    return 1;
}

static int handle_luainit (lua_State *L) {
    const char *name = "=" LUA_INITVARVERSION;
    const char *init = getenv(name + 1);
    if (init == NULL) {
        name = "=" LUA_INIT_VAR;
        init = getenv(name + 1);  /* try alternative name */
    }
    if (init == NULL) return LUA_OK;
    else if (init[0] == '@')
        return dofile(L, init+1);
    else
        return dostring(L, init, name);
}

/*
** {==================================================================
** Read-Eval-Print Loop (REPL)
** ===================================================================
*/

#if !defined(LUA_PROMPT)
#define LUA_PROMPT		"> "
#define LUA_PROMPT2		">> "
#endif

#if !defined(LUA_MAXINPUT)
#define LUA_MAXINPUT		512
#endif

/*
** lua_stdin_is_tty detects whether the standard input is a 'tty' (that
** is, whether we're running lua interactively).
*/
#if !defined(lua_stdin_is_tty)	/* { */
#include <unistd.h>

#define lua_stdin_is_tty()	isatty(0)
#endif				/* } */


/*
** lua_readline defines how to show a prompt and then read a line from
** the standard input.
** lua_saveline defines how to "save" a read line in a "history".
** lua_freeline defines how to free a line read by lua_readline.
*/
#if defined(LUA_USE_READLINE)	/* { */

#include <readline/readline.h>
#include <readline/history.h>
#define lua_initreadline(L)	((void)L, rl_readline_name="bundled-lua")
#define lua_readline(L,b,p)	((void)L, ((b)=readline(p)) != NULL)
#define lua_saveline(L,line)	((void)L, add_history(line))
#define lua_freeline(L,b)	((void)L, free(b))

#else				/* }{ */

#define lua_initreadline(L)  ((void)L)
#define lua_readline(L,b,p)                                         \
    ((void)L, fputs(p, stdout), fflush(stdout),  /* show prompt */  \
     fgets(b, LUA_MAXINPUT, stdin) != NULL)  /* get line */
#define lua_saveline(L,line)	{ (void)L; (void)line; }
#define lua_freeline(L,b)	{ (void)L; (void)b; }

#endif				/* } */

/*
** Return the string to be used as a prompt by the interpreter. Leave
** the string (or nil, if using the default value) on the stack, to keep
** it anchored.
*/
static const char *get_prompt (lua_State *L, int firstline) {
    if (lua_getglobal(L, firstline ? "_PROMPT" : "_PROMPT2") == LUA_TNIL)
        return (firstline ? LUA_PROMPT : LUA_PROMPT2);  /* use the default */
    else {  /* apply 'tostring' over the value */
        const char *p = luaL_tolstring(L, -1, NULL);
        lua_remove(L, -2);  /* remove original value */
        return p;
    }
}

/* mark in error messages for incomplete statements */
#define EOFMARK		"<eof>"
#define marklen		(sizeof(EOFMARK)/sizeof(char) - 1)

/*
** Check whether 'status' signals a syntax error and the error
** message at the top of the stack ends with the above mark for
** incomplete statements.
*/
static int incomplete (lua_State *L, int status) {
    if (status == LUA_ERRSYNTAX) {
        size_t lmsg;
        const char *msg = lua_tolstring(L, -1, &lmsg);
        if (lmsg >= marklen && strcmp(msg + lmsg - marklen, EOFMARK) == 0) {
            lua_pop(L, 1);
            return 1;
        }
    }
    return 0;  /* else... */
}

/*
** Prompt the user, read a line, and push it into the Lua stack.
*/
static int pushline (lua_State *L, int firstline) {
    char buffer[LUA_MAXINPUT];
    char *b = buffer;
    size_t l;
    const char *prmt = get_prompt(L, firstline);
    int readstatus = lua_readline(L, b, prmt);
    if (readstatus == 0)
        return 0;  /* no input (prompt will be popped by caller) */
    lua_pop(L, 1);  /* remove prompt */
    l = strlen(b);
    if (l > 0 && b[l-1] == '\n')  /* line ends with newline? */
        b[--l] = '\0';  /* remove it */
    if (firstline && b[0] == '=')  /* for compatibility with 5.2, ... */
        lua_pushfstring(L, "return %s", b + 1);  /* change '=' to 'return' */
    else
        lua_pushlstring(L, b, l);
    lua_freeline(L, b);
    return 1;
}

/*
** Try to compile line on the stack as 'return <line>;'; on return, stack
** has either compiled chunk or original line (if compilation failed).
*/
static int addreturn (lua_State *L) {
    const char *line = lua_tostring(L, -1);  /* original line */
    const char *retline = lua_pushfstring(L, "return %s;", line);
    int status = luaL_loadbuffer(L, retline, strlen(retline), "=stdin");
    if (status == LUA_OK) {
        lua_remove(L, -2);  /* remove modified line */
        if (line[0] != '\0')  /* non empty? */
            lua_saveline(L, line);  /* keep history */
    }
    else
        lua_pop(L, 2);  /* pop result from 'luaL_loadbuffer' and modified line */
    return status;
}

/*
** Read multiple lines until a complete Lua statement
*/
static int multiline (lua_State *L) {
    for (;;) {  /* repeat until gets a complete statement */
        size_t len;
        const char *line = lua_tolstring(L, 1, &len);  /* get what it has */
        int status = luaL_loadbuffer(L, line, len, "=stdin");  /* try it */
        if (!incomplete(L, status) || !pushline(L, 0)) {
            lua_saveline(L, line);  /* keep history */
            return status;  /* cannot or should not try to add continuation line */
        }
        lua_pushliteral(L, "\n");  /* add newline... */
        lua_insert(L, -2);  /* ...between the two lines */
        lua_concat(L, 3);  /* join them */
    }
}

/*
** Read a line and try to load (compile) it first as an expression (by
** adding "return " in front of it) and second as a statement. Return
** the final status of load/call with the resulting function (if any)
** in the top of the stack.
*/
static int loadline (lua_State *L) {
    int status;
    lua_settop(L, 0);
    if (!pushline(L, 1))
        return -1;  /* no input */
    if ((status = addreturn(L)) != LUA_OK)  /* 'return ...' did not work? */
        status = multiline(L);  /* try as command, maybe with continuation lines */
    lua_remove(L, 1);  /* remove line from the stack */
    lua_assert(lua_gettop(L) == 1);
    return status;
}

/*
** Prints (calling the Lua 'print' function) any values on the stack
*/
static void l_print (lua_State *L) {
    int n = lua_gettop(L);
    if (n > 0) {  /* any result to be printed? */
        luaL_checkstack(L, LUA_MINSTACK, "too many results to print");
        lua_getglobal(L, "print");
        lua_insert(L, 1);
        if (lua_pcall(L, n, 0, 0) != LUA_OK)
            l_message(progname, lua_pushfstring(L, "error calling 'print' (%s)",
                                                lua_tostring(L, -1)));
    }
}

/*
** Do the REPL: repeatedly read (load) a line, evaluate (call) it, and
** print any results.
*/
static void doREPL (lua_State *L) {
    int status;
    const char *oldprogname = progname;
    progname = NULL;  /* no 'progname' on errors in interactive mode */
    lua_initreadline(L);
    while ((status = loadline(L)) != -1) {
        if (status == LUA_OK)
            status = docall(L, 0, LUA_MULTRET);
        if (status == LUA_OK) l_print(L);
        else report(L, status);
    }
    lua_settop(L, 0);  /* clear stack */
    lua_writeline();
    progname = oldprogname;
}

/* }================================================================== */

/*
** Main body of stand-alone interpreter (to be called in protected mode).
** Reads the options and handles them all.
*/
static int pmain (lua_State *L) {
    int argc = (int)lua_tointeger(L, 1);
    char **argv = (char **)lua_touserdata(L, 2);
    int script;
    int args = collectargs(argv, &script);
    int optlim = (script > 0) ? script : argc; /* first argv not an option */
    luaL_checkversion(L);  /* check that interpreter has correct version */
    if (args == has_error) {  /* bad arg? */
        print_usage(argv[script]);  /* 'script' has index of bad arg. */
        return 0;
    }
    if (args & has_v)  /* option '-v'? */
        print_version();
    if (args & has_E) {  /* option '-E'? */
        lua_pushboolean(L, 1);  /* signal for libraries to ignore env. vars. */
        lua_setfield(L, LUA_REGISTRYINDEX, "LUA_NOENV");
    }
    luaL_openlibs(L);  /* open standard libraries */
    make_index();
    fill_state_with_luaposix(L);
    fill_state_with_bundled(L);
    fill_state_with_initless(L);
    createargtable(L, argv, argc, script);  /* create table 'arg' */
    lua_gc(L, LUA_GCRESTART);  /* start GC... */
    lua_gc(L, LUA_GCGEN, 0, 0);  /* ...in generational mode */
    if (!(args & has_E)) {  /* no option '-E'? */
        if (handle_luainit(L) != LUA_OK)  /* run LUA_INIT */
            return 0;  /* error running LUA_INIT */
    }
    if (!runargs(L, argv, optlim))  /* execute arguments -e and -l */
        return 0;  /* something failed */
    if (script > 0) {  /* execute main script (if there is one) */
        if (handle_script(L, argv + script) != LUA_OK)
            return 0;  /* interrupt in case of error */
    }
    if (args & has_i)  /* -i option? */
        doREPL(L);  /* do read-eval-print loop */
    else if (script < 1 && !(args & (has_e | has_v))) { /* no active option? */
        if (lua_stdin_is_tty()) {  /* running in interactive mode? */
            print_version();
            doREPL(L);  /* do read-eval-print loop */
        }
        else dofile(L, NULL);  /* executes stdin as a file */
    }
    lua_pushboolean(L, 1);  /* signal no errors */
    return 1;
}

int main (int argc, char **argv) {
    int status, result;
    lua_State *L = luaL_newstate();  /* create state */
    if (L == NULL) {
        l_message(argv[0], "cannot create state: not enough memory");
        return EXIT_FAILURE;
    }
    lua_gc(L, LUA_GCSTOP);  /* stop GC while building state */
    lua_pushcfunction(L, &pmain);  /* to call 'pmain' in protected mode */
    lua_pushinteger(L, argc);  /* 1st argument */
    lua_pushlightuserdata(L, argv); /* 2nd argument */
    status = lua_pcall(L, 2, 1, 0);  /* do the call */
    result = lua_toboolean(L, -1);  /* get result */
    report(L, status);
    lua_close(L);
    return (result && status == LUA_OK) ? EXIT_SUCCESS : EXIT_FAILURE;
}
