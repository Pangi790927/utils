/*!
 * @file
 * @brief What virt_composer_plugins.h declares: the loader, which opens plugins for the process and
 * registers them into states, the owners of names, and add_plugin_internal_func.
 *
 * Detail:
 *   - Every program that links virt_composer.cpp links this too, plugins included: every
 *     registration reads the owners of names, a host's as much as a plugin's, and a plugin's
 *     registration calls add_plugin_internal_func. Only a host calls the loader.
 *   - It is one of the library's .cpp files, so it reads virt_state_t whole, through
 *     virt_composer_internal.h, and claims names through may_claim_name, which virt_composer.cpp
 *     defines.
 *
 * @date 06-10-2026-02:45
 */

#include <filesystem>
#include <map>
#include <set>
#include <string>

#include "virt_composer.h"
/* virt_state_t and what the library's .cpp files share. This file is one of them.
06-10-2026-00:40 */
#define VIRT_COMPOSER_LIBRARY
#include "virt_composer_internal.h"
#include "virt_composer_plugins.h"

namespace virt_composer
{

/* [INTERNAL] The one platform difference in opening a module, kept to a few lines so the loader
and name_owner() below read the same everywhere. RTLD_LOCAL
is not incidental: it is what keeps each plugin's _type_offset its own, since two plugins both
define that symbol and RTLD_GLOBAL would let the first one's satisfy the second's lookups. A plugin
carries its own copy of this library and reaches nothing of its host's but what VC_API marks, which
it finds through module_main(), the main program as a handle. 28-09-2026-12:00
A handle is a void * on both systems, an HMODULE being a pointer. Moved here from
virt_composer.cpp, since this file alone opens modules now. 06-10-2026-02:45 */
#if defined(UTILS_OS_WINDOWS)
static void *module_open(const char *p)         { return (void *)LoadLibraryA(p); }
static void *module_sym(void *h, const char *n) { return (void *)GetProcAddress((HMODULE)h, n); }
static void module_close(void *h)               { FreeLibrary((HMODULE)h); }
static void *module_main()                      { return (void *)GetModuleHandleA(nullptr); }
#else
static void *module_open(const char *p)         { return dlopen(p, RTLD_NOW|RTLD_LOCAL); }
static void *module_sym(void *h, const char *n) { return dlsym(h, n); }
static void module_close(void *h)               { dlclose(h); }
static void *module_main()                      { return dlopen(nullptr, RTLD_NOW); }
#endif

/* [INTERNAL] The range of type ids a plugin owns, by real path, and where the next plugin's range
will begin. A range belongs to the process rather than to any one state, and is never given back.

It has to be the process's, because a plugin holds one _type_offset for all of its types: were two
states to hand it different offsets, whichever registered last would silently move the ids of the
objects the other had already made. Assigning the range once and having every state adopt it is
what lets a plugin serve more than one of them. It works because VIRT_TYPE_CNT is a constant of the
host binary, so every state in the process begins the same size and an offset means the same thing
in all of them. 2026-09-20 17:45 */
struct plugin_range_t {
    size_t off;
    size_t cnt;
};
static std::map<std::string, plugin_range_t> plugin_ranges;
static size_t next_plugin_offset = 0;

/* [INTERNAL] The plugins already open in this process, by real path, with what the host needs from
each. A plugin is opened and checked once and kept for good, so asking for it again costs neither
an open nor a version check, and every state that asks registers from the same entry.

Deliberately keyed by path and holding no virt_state_t: a state is a heap address, a rebuilt one
routinely lands on the address a destroyed one had, and a table out here remembering "this plugin
is already in that state" would then skip registering into a state that has nothing. What a state
has is the state's own to know, and virt_state_t::loaded_plugins knows it. 2026-09-20 18:55
It also keeps the plugin's logfile, resolved, its plugin_uninit, and whether that is still to run.
05-10-2026-22:33 */
struct plugin_t {
    void *handle;
    size_t type_cnt;
    int (*register_meta)(virt_state_t *, int);
    void (*uninit)();
    std::string logfile;
    bool inited;
};
static std::map<std::string, plugin_t> open_plugins;

/* [INTERNAL] The plugins that could not be used, by real path. Only what cannot be undone is
remembered: a plugin that spent a range and then failed to register. Everything that goes wrong
before the range is spent is left out and the library is closed again, so correcting the plugin and
asking once more genuinely retries - closing drops the last reference, and the next open reads the
file from disk instead of handing back the copy already in the process. 2026-09-20 17:45
The range is spent by load_plugin(), and a plugin lands here when register_plugin() fails it.
05-10-2026-22:33 */
static std::set<std::string> broken_plugins;

/* [INTERNAL] Gives a plugin the range of type ids it owns for the life of the process, after the
host's types and every range already given. Called once per plugin, by load_plugin().
05-10-2026-22:33 */
static void take_plugin_range(const std::string& real, size_t cnt) {
    if (!next_plugin_offset)
        next_plugin_offset = VIRT_TYPE_CNT;
    plugin_ranges[real] = plugin_range_t{next_plugin_offset, cnt};
    next_plugin_offset += cnt;
}

/* [INTERNAL] Grows this state's per-type containers far enough to cover a plugin's range. A state
that skipped an earlier plugin's range still has to reach past it, and carries unused rows there.
The room is never given back. 2026-09-20 17:45, 05-10-2026-22:33 */
static void grow_state(virt_state_t *vs, const plugin_range_t& range) {
    size_t need = range.off + range.cnt;
    if (max_type_cnt(vs) >= need)
        return;

    vs->trivial_copy_member.resize(need);
    vs->lua_class_members.resize(need);
    vs->lua_class_member_setters.resize(need);
    vs->lua_class_operators.resize(need);
    vs->inheritance_table.resize(need);
}

/* [INTERNAL] The five exports a plugin must have, as open_plugin reads them. 05-10-2026-22:33 */
struct plugin_exports_t {
    const char *(*version)();
    int (*type_cnt)();
    int (*register_meta)(virt_state_t *, int);
    int (*init)(const char *);
    void (*uninit)();
};

/* [INTERNAL] Reads the five exports, and answers -1 having said so when one is missing.
05-10-2026-22:33 */
static int read_exports(void *handle, const std::string& real, plugin_exports_t& ex) {
    ex.version       = (const char *(*)())module_sym(handle, "plugin_get_version");
    ex.type_cnt      = (int (*)())module_sym(handle, "plugin_type_cnt");
    ex.register_meta = (int (*)(virt_state_t *, int))module_sym(handle, "plugin_register_meta");
    ex.init          = (int (*)(const char *))module_sym(handle, "plugin_init");
    ex.uninit        = (void (*)())module_sym(handle, "plugin_uninit");
    if (!ex.version || !ex.type_cnt || !ex.register_meta || !ex.init || !ex.uninit) {
        DBG("Not a plugin, one of plugin_get_version/plugin_type_cnt/plugin_register_meta/"
                "plugin_init/plugin_uninit is missing: %s", real.c_str());
        return -1;
    }
    return 0;
}

/* [INTERNAL] Checks the plugin's version and type count, and answers -1 having said why it cannot
be used. 05-10-2026-22:33 */
static int check_exports(const plugin_exports_t& ex, const std::string& real) {
    if (strcmp(ex.version(), VIRT_COMPOSER_ABI) != 0) {
        DBG("Plugin %s was built against another virt_composer:\n  plugin: %s\n  host:   %s",
                real.c_str(), ex.version(), VIRT_COMPOSER_ABI);
        return -1;
    }
    if (ex.type_cnt() < 0) {
        DBG("Plugin %s reports a negative type count: %d", real.c_str(), ex.type_cnt());
        return -1;
    }
    return 0;
}

/* [INTERNAL] Answers whether `logfile` is already a log of this process: the host's, or a loaded
plugin's. Both sides are resolved first, so two spellings of one file are one. 05-10-2026-22:33 */
static bool log_taken(const std::string& logfile) {
    std::error_code ec;
    auto want = std::filesystem::weakly_canonical(logfile, ec);
    auto same = [&](const std::string& other) {
        return std::filesystem::weakly_canonical(other, ec) == want;
    };
    if (same(logger_stem()))
        return true;
    for (auto &[real, plug] : open_plugins)
        if (same(plug.logfile))
            return true;
    return false;
}

/* [INTERNAL] Opens a plugin not yet open and satisfies itself that it can be used, or answers null
having said why. Everything that fails here fails before any range is taken, so the library is
closed again and correcting the plugin and asking once more genuinely retries. 2026-09-20 17:45
plugin_init is called last, so nothing after it can fail: a plugin whose log is open is kept.
05-10-2026-22:33 */
static plugin_t *open_plugin(const std::string& real, const std::string& logfile) {
    if (log_taken(logfile)) {
        DBG("Plugin %s cannot log to %s, another logger writes there", real.c_str(),
                logfile.c_str());
        return nullptr;
    }

    void *handle = module_open(real.c_str());
    if (!handle) {
        DBG("Could not open plugin: %s", real.c_str());
        return nullptr;
    }

    plugin_exports_t ex;
    if (read_exports(handle, real, ex) < 0 || check_exports(ex, real) < 0) {
        module_close(handle);
        return nullptr;
    }
    if (ex.init(logfile.c_str()) < 0) {
        DBG("Plugin %s could not open its log %s", real.c_str(), logfile.c_str());
        ex.uninit();
        module_close(handle);
        return nullptr;
    }

    open_plugins[real] = plugin_t{handle, (size_t)ex.type_cnt(), ex.register_meta, ex.uninit,
            logfile, true};
    return &open_plugins[real];
}

/* [INTERNAL] Answers in `real` the one path every spelling of a plugin's path resolves to, or -1
having said there is no such file. 05-10-2026-22:33 */
static int real_path(const char *path, std::string& real) {
    std::error_code ec;
    real = std::filesystem::canonical(path, ec).string();
    if (ec) {
        DBG("No such plugin: %s [%s]", path, ec.message().c_str());
        return -1;
    }
    return 0;
}

/* See load_plugin()'s declaration in virt_composer_plugins.h for its doc comment. */
int load_plugin(const char *path, const char *logfile) {
    if (!logfile || !*logfile) {
        DBG("Plugin %s was given no logfile", path);
        return -1;
    }
    std::string real;
    if (real_path(path, real) < 0)
        return -1;

    /* Resolved here, in the host, so a relative logfile is taken from the host's directory. The
    plugin's own path_get_relative() would take it from the plugin's. 05-10-2026-22:33 */
    std::string log = path_get_relative(logfile);
    if (has(open_plugins, real)) {
        if (open_plugins[real].logfile == log)
            return 0;
        DBG("Plugin %s logs to %s already, it cannot also log to %s", real.c_str(),
                open_plugins[real].logfile.c_str(), log.c_str());
        return -1;
    }

    plugin_t *plug = open_plugin(real, log);
    if (!plug)
        return -1;
    take_plugin_range(real, plug->type_cnt);
    return 0;
}

/* See register_plugin()'s declaration in virt_composer_plugins.h for its doc comment. */
int register_plugin(virt_state_t *vs, const char *path) {
    std::string real;
    if (real_path(path, real) < 0)
        return -1;
    if (!has(open_plugins, real)) {
        DBG("Plugin %s is not loaded, load_plugin() comes first", real.c_str());
        return -1;
    }
    if (has(broken_plugins, real)) {
        DBG("Plugin %s took a range and then failed to register, it is not asked again",
                real.c_str());
        return -1;
    }

    if (has(vs->loaded_plugins, real))
        return 0;

    plugin_t& plug = open_plugins[real];
    const plugin_range_t& range = plugin_ranges[real];
    grow_state(vs, range);

    /* Marked where the state grows to hold the range, not where registering finishes. From this
    line the state holds this plugin's ids whether the registration below succeeds or not, so this
    is the moment it is true. 2026-09-20 19:10 */
    vs->loaded_plugins.insert(real);

    /* The plugin is named for as long as it registers, so every name it claims is recorded as its
    own and a name already belonging to another is refused rather than replaced. 2026-09-20 19:30 */
    vs->registering_plugin = &real;
    vs->name_conflict = false;
    int reg = plug.register_meta(vs, (int)range.off);
    vs->registering_plugin = nullptr;

    if (reg < 0 || vs->name_conflict) {
        DBG("Plugin %s was refused: %s. Its range stays unused.", real.c_str(),
                vs->name_conflict ? "it claimed a name another already owns"
                                  : "plugin_register_meta failed");
        broken_plugins.insert(real);
        return -1;
    }
    return 0;
}

/* See uninit_plugins()'s declaration in virt_composer_plugins.h for its doc comment. */
void uninit_plugins() {
    for (auto &[real, plug] : open_plugins) {
        if (!plug.inited)
            continue;
        plug.uninit();
        plug.inited = false;
    }
}


/* [INTERNAL] Who owns each registered name, by name, with the empty string for the host. A name
has one owner and keeps it for the life of the process.

Process-wide, and it has to be, because the tables it guards are: the internal-function table
belongs to the module, not to any one state, so an owner map that died with a state would let the
next state hand a name to someone else and change what the first state answers. Checked
20-09-2026: a per-state map let exactly that through. 2026-09-20 19:45
It is the host's: exported under a C name, so that a plugin, which registers through its own copy of
this library, finds the host's table and not its own. 28-09-2026-12:00 */
extern "C" VC_API void *vc_host_name_owner() {
    static std::map<std::string, std::string> owners;
    return &owners;
}

/* See name_owner()'s declaration in virt_composer_plugins.h for its doc comment. Every module
defines vc_host_name_owner, the host and each plugin alike, and every module asks the main program
for its: so all of them answer the host's table. A host that does not show the symbol -- a Linux
one linked without naming it -- answers each module its own, and says so once. 28-09-2026-12:00,
06-10-2026-01:30 */
std::map<std::string, std::string> &name_owner() {
    using owners_t = std::map<std::string, std::string>;
    static owners_t *owners = [] {
        auto fn = (void *(*)())module_sym(module_main(), "vc_host_name_owner");
        if (!fn) {
            DBG("The main program does not show vc_host_name_owner: names are checked against "
                    "this module's own registrations only");
            return (owners_t *)vc_host_name_owner();
        }
        return (owners_t *)fn();
    }();
    return *owners;
}

/* See add_plugin_internal_func()'s declaration in virt_composer_plugins.h for its doc comment.
It was a member of c_function_t, and is a function of the plugin component now. 06-10-2026-01:30 */
void add_plugin_internal_func(virt_state_t *vs, std::string name,
        std::function<int(lua_State *L)> fn)
{
    if (!may_claim_name(vs, name))
        return;
    (*vs->internal_funcs)[name] = fn;
}

} /* namespace virt_composer */
