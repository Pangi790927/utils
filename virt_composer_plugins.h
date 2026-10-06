#ifndef VIRT_COMPOSER_PLUGINS_H
#define VIRT_COMPOSER_PLUGINS_H

/*!
 * @file
 * @brief Plugins: shared objects a host loads at run time, each bringing types and functions of
 * its own into the states the host registers it into.
 *
 * Core:
 *   - A host loads a plugin once for the process with @ref load_plugin, and puts it into each
 *     state that wants it with @ref register_plugin. @ref uninit_plugins closes their logs last.
 *   - A plugin's final file marks its five exports with VIRT_COMPOSER_PLUGIN_EXPORT, declares its
 *     types with VIRT_COMPOSER_REGISTER_PLUGIN_TYPE, and counts them under @ref plugin_tag_t.
 *     utils/tests/virt_composer/plugins/reference_plugin.cpp is the shape to copy.
 *
 * Detail:
 *   - This file is a component of virt_composer, as virt_composer_coroutines.h is: it is included
 *     right after virt_composer.h, by a host that loads plugins and by a plugin alike. It
 *     registers no type, so where it stands among the registrations changes no id.
 *   - Its .cpp, virt_composer_plugins.cpp, is linked by every program that links virt_composer.cpp,
 *     plugins included: every registration reads the owners of names, a host's as much as a
 *     plugin's, and a plugin's registration calls add_plugin_internal_func. Only a host calls the
 *     loader in it.
 *   - VIRT_COMPOSER_PLUGIN_COUNTERS is documented here, and defined, when it is, before
 *     virt_composer.h; virt_composer_end.h is what reads it.
 *
 * @date 06-10-2026-00:40
 */

#ifndef VIRT_COMPOSER_H
# error "virt_composer_plugins.h is a component of virt_composer and is included right after \
virt_composer.h - see the file comment."
#endif

/*!
 * @def VC_API
 * @brief Marks what a module shows the rest of the process: the rare function the host owns and
 * every plugin must reach the host's copy of, rather than its own.
 *
 * Core:
 *   - Every module -- the host and each plugin -- carries its own copy of virt_composer, and the
 *     ABI hash is what keeps them in agreement. Almost nothing needs more than that; what does is
 *     marked with this, and is looked up in the main program when it is needed (see
 *     `name_owner()`), so a plugin finds the host's copy whatever the program is called.
 *   - It exports: `dllexport` on Windows, and on Linux default visibility, which a host also has to
 *     name at link time with `-Wl,--export-dynamic-symbol`, since an executable shows nothing by
 *     default.
 *
 * @date 28-09-2026-12:00
 */
#if defined(UTILS_OS_WINDOWS)
# define VC_API __declspec(dllexport)
#else
# define VC_API __attribute__((visibility("default")))
#endif

/*!
 * @def VIRT_COMPOSER_PLUGIN_COUNTERS
 * @brief Marks a translation unit as a plugin's, so that it counts its types without publishing
 * the answer.
 *
 * Core:
 *   - Left undefined, the translation unit is a host's: `virt_composer_end.h` publishes the type
 *     count in `VIRT_TYPE_CNT` and raises `VIRT_TYPES_INITIALIZED`, which is what `create_state()`
 *     reads and what it refuses to run without.
 *   - Defined, the translation unit is a plugin's: the counting still happens and still closes the
 *     registrations, but nothing is published. A plugin answers for its own types through
 *     `plugin_type_cnt()` instead, and the host asks it there.
 *   - Only whether it is defined matters, not what it is defined to. It must be defined before
 *     this header is included.
 *
 * Detail:
 *   - Each module carries its own copy of this library, the counters included, and only the
 *     host's are ever read: by the host's `create_state()` and by `load_plugin()`, which places a
 *     plugin's range after the host's types. A plugin's own copies stay unread, so its count
 *     travels through `plugin_type_cnt()` alone.
 *
 * @see load_plugin, VIRT_COMPOSER_REGISTER_PLUGIN_TYPE
 *
 * @date 28-09-2026-12:00
 */
#ifndef VIRT_COMPOSER_PLUGIN_COUNTERS
// Nothing here, this is only for documentation purposes
#endif

/*!
 * @def VIRT_COMPOSER_PLUGIN_EXPORT
 * @brief Marks the five functions a plugin puts on the outside for the host to find by name:
 * `plugin_get_version`, `plugin_type_cnt`, `plugin_register_meta`, `plugin_init` and
 * `plugin_uninit`.
 *
 * Core:
 *   - It is `extern "C"` and exported: on Windows a DLL shows nothing it was not told to show, and
 *     on Linux it stays visible however the plugin is compiled.
 *
 * @date 05-10-2026-22:33
 */
#if defined(UTILS_OS_WINDOWS)
# define VIRT_COMPOSER_PLUGIN_EXPORT extern "C" __declspec(dllexport)
#else
# define VIRT_COMPOSER_PLUGIN_EXPORT extern "C" __attribute__((visibility("default")))
#endif

/*!
 * @def VIRT_COMPOSER_REGISTER_PLUGIN_TYPE(type)
 * @brief Registers a plugin's object type, as a function answering the id rather than a constant.
 *
 * Core:
 *   - Defines `type()`, which answers the `object_type_e` the object inheriting `vc::object_t` is
 *     expected to return from `type_id()` and `type_id_static()`. It is called, not read:
 *     `VC_TYPE_FOO()` where a host's type would be written `VC_TYPE_FOO`.
 *   - The id it answers is `_type_offset` plus an index counted under
 *     `virt_composer::plugin_tag_t`. The plugin's translation unit must define `_type_offset`, and
 *     one that does not fails to link, so the offset cannot be left out quietly.
 *   - It answers correctly only once the host has set `_type_offset`, which `register_plugin()`
 *     does before it lets the plugin register. Read before that, every id is its bare index and
 *     collides with the host's built-in types.
 *   - It also defines `type##_local`, the index on its own, so two types whose names differ only
 *     by that suffix collide.
 *
 * Detail:
 *   - A function rather than a constant precisely because of the ordering above. A namespace-scope
 *     constant is initialised while the shared object loads, which is before the host can hand
 *     over an offset, so it would capture 0 and say nothing about it.
 *   - `plugin_tag_t` counts from 0 independently of `virt_tag_t`, so a plugin's first type is its
 *     index 0 and the offset needs no adjustment for the six types this header registers.
 *
 * @param type The enum name associated to a type to register as an enumerator.
 *
 * @see VIRT_COMPOSER_REGISTER_TYPE, plugin_tag_t, load_plugin, register_plugin
 *
 * @date 2026-09-20 16:07
 */
#define VIRT_COMPOSER_REGISTER_PLUGIN_TYPE(type) \
        constexpr int type##_local = \
                virt_object::compile_unique_id<virt_composer::plugin_tag_t>(); \
        inline virt_composer::object_type_e type() { \
            return virt_composer::object_type_e{_type_offset + type##_local, #type}; }

namespace virt_composer {

/*!
 * @brief Counts the type ids belonging to a plugin, separately from its host's.
 *
 * Core:
 *   - A plugin's types count from 0 under this tag while the built-in types count from 0 under
 *     `virt_tag_t`, so the two never interleave and a plugin's first type is its index 0. What
 *     keeps a plugin's ids clear of its host's is the offset added on top, not this tag.
 *   - Each shared object counts on its own, the counter being per translation unit, so one tag
 *     serves every plugin.
 *
 * @see virt_tag_t, VIRT_COMPOSER_REGISTER_PLUGIN_TYPE, load_plugin
 *
 * @date 2026-09-20 16:07
 */
struct plugin_tag_t {};

/*!
 * @brief Loads a plugin for the whole process: opens it, opens its log and gives it a private
 * range of type ids. No state is touched; see @ref register_plugin for that.
 *
 * Core:
 *   - The plugin must export `plugin_get_version`, `plugin_type_cnt`, `plugin_register_meta`,
 *     `plugin_init` and `plugin_uninit`, each marked VIRT_COMPOSER_PLUGIN_EXPORT.
 *     `plugin_get_version` answers the plugin's own VIRT_COMPOSER_ABI and never the host's, which
 *     would agree with the host whatever the plugin was built against. It must have been built
 *     against this same virt_composer. A plugin built against another is refused.
 *   - The plugin logs into a file of its own, `logfile`: it writes `<logfile>.log` and
 *     `<logfile>.old.log`, as the host's logger does. A relative logfile is taken from the host's
 *     directory. The plugin's `plugin_init` opens it here, once. What a plugin must do once per
 *     process belongs in `plugin_init`.
 *   - A logfile that is the host's own log, or another loaded plugin's, is refused: two loggers on
 *     one file would each rotate it without the other.
 *   - Loading a plugin already loaded, by any path that resolves to the same file, does nothing
 *     and answers success when the logfile is the same, and is refused when it is not.
 *   - The range a plugin takes is its own for the life of the process, so its types carry one id
 *     in every state it is registered into. A load that fails leaves nothing behind, so the plugin
 *     may be corrected and loaded again.
 *   - A plugin does not load a plugin. Only the host calls this, and a plugin that called it
 *     would be reaching for bookkeeping that belongs to the host - which on a platform where a
 *     plugin carries its own copy of this library is its own, and would hand out ranges from a
 *     counter the host knows nothing about.
 *
 * @param path     The plugin's path, in any spelling that resolves to the file.
 * @param logfile  Where the plugin logs, without the ".log"; relative to the host's directory
 *                 unless absolute. Not null, not empty.
 *
 * @return 0 when the plugin is loaded, -1 otherwise.
 *
 * @date 05-10-2026-22:33
 */
int load_plugin(const char *path, const char *logfile);

/*!
 * @brief Registers a loaded plugin's types and functions into a state.
 *
 * Core:
 *   - The plugin must have been loaded by @ref load_plugin first; one that was not is refused.
 *   - Enlarges the state to fit the plugin's range, hands the plugin the offset of that range and
 *     lets it register. Afterwards its types are constructible from YAML and usable from Lua like
 *     any other.
 *   - Each state that wants a plugin registers it itself; a state made after a load gets nothing
 *     by being made.
 *   - Registering a plugin a state already has, by any path that resolves to the same file, does
 *     nothing and answers success.
 *   - A plugin that fails to register, or claims a name another already owns, is not registered
 *     into any state again.
 *   - `plugin_register_meta` runs once per state, not once per process. A plugin that does
 *     something there which is not about the state it is handed will find it happening again for
 *     the next one.
 *   - A state pays for the ranges it skips. Registering only the second of two plugins still grows
 *     the state past the first's range, leaving rows nothing will ever index.
 *
 * @warning A plugin's types do not inherit from its host's. A plugin type may derive from a host
 *       type in C++, but the base's members are not carried over to it, and nothing says so at the
 *       point it fails - the member is simply absent in Lua.
 * @warning A plugin must not keep a `vc::ref_t` in static or global storage. It outlives the state
 *       it came from, and releasing it at process exit reaches through a dead `lua_State`.
 *
 * @param vs    The state to register into.
 * @param path  The plugin's path, in any spelling that resolves to the file it was loaded from.
 *
 * @return 0 when the plugin is registered into the state, -1 otherwise.
 *
 * @date 05-10-2026-22:33
 */
int register_plugin(virt_state_t *vs, const char *path);

/*!
 * @brief Calls every loaded plugin's `plugin_uninit` once, which closes each plugin's log.
 *
 * Core:
 *   - Call it last, when no state will use a plugin again: the plugins stay loaded, and a plugin
 *     that logs afterwards opens a log of its own beside its file instead.
 *   - Calling it again does nothing.
 *
 * @date 05-10-2026-22:33
 */
void uninit_plugins();

/*! Answers who owns each registered name -- an `[INTERNAL]` function's, a named builder
 * callback's -- by name, with the empty string for the host. There is one table for the whole
 * program, the host's, and the host and every plugin answer that one, so a plugin cannot take a
 * name the host or another plugin already holds. @date 28-09-2026-12:00 */
std::map<std::string, std::string> &name_owner();

/*! Registers a callback into the table the given state binds its `[INTERNAL]` names from.
 *
 * Core:
 *   - The same as c_function_t::add_internal_func() except for which table it fills, and a plugin
 *     must use this one.
 *   - A plugin carries its own copy of this library, and add_internal_func() would fill that
 *     copy's table, which no state reads. Naming the state reaches the table the state binds
 *     from, the host's.
 *
 * @date 28-09-2026-12:00, 06-10-2026-01:30 */
void add_plugin_internal_func(virt_state_t *vs, std::string name,
        std::function<int(lua_State *L)> fn);

} /* namespace virt_composer */

#endif /* VIRT_COMPOSER_PLUGINS_H */
