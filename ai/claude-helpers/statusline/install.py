#!/usr/bin/env python3
"""@file Installs statusline.py into the Claude Code user settings, as the status line, refreshed
every second (Linux; windows/install.py installs the status line of Windows).

The settings file is ~/.claude/settings.json, or settings.json under CLAUDE_CONFIG_DIR when that
is set. The command written names statusline.py by its absolute path in this checkout, so a PC
that keeps the repo elsewhere works too; after the checkout moves, a new run points it at the new
place. Every other setting is kept, and a run with nothing to change writes nothing. A status
line that is not this one is replaced only with --replace; its other options, such as padding,
are kept. The PreCompact and PostCompact hooks that the install of 27-10-2026-20:40 put in are
taken out. Before a change, the file as it was is copied to settings.json.bak. Reading and
writing the settings is claude_settings.py's.

Usage:
    install.py [--replace] [--dry]

@date 27-10-2026-20:40, 27-10-2026-21:40, 28-10-2026-11:35
"""

import os
import shlex
import sys

import claude_settings

HERE = os.path.dirname(os.path.abspath(__file__))
STATUSLINE = os.path.join(HERE, "statusline.py")
# Every command this writes names this file; one that names it from another checkout is still
# recognized as this one, and replaced. 27-10-2026-20:40
NAME = "statusline/statusline.py"
# The events whose hooks the install of 27-10-2026-20:40 added, with the argument of each.
# 27-10-2026-21:40
OLD_HOOKS = {"PreCompact": "start", "PostCompact": "end"}


def command(*args):
    """Returns the shell command that runs statusline.py with `args`.

    @param args  str - its arguments
    @return str
    @date 27-10-2026-20:40
    """
    return " ".join(["python3", shlex.quote(STATUSLINE), *args])


def is_ours(cmd, *args):
    """Tells whether the command `cmd` runs a statusline.py, of any checkout, with `args`.

    @param cmd   str - a command from the settings
    @param args  str - the arguments it must end with
    @return bool
    @date 27-10-2026-20:40
    """
    if not isinstance(cmd, str) or NAME not in cmd:
        return False
    return cmd.split(NAME, 1)[1].strip(" '\"").split() == list(args)


def drop_old_hook(settings, event, which):
    """Takes the hook statusline.py --compact `which` of any checkout out of `event`, with the
    group and the event it leaves empty; the other hooks stay as they are.

    @param settings  dict - the settings, changed in place
    @param event     str - "PreCompact" or "PostCompact"
    @param which     str - "start" or "end"
    @date 27-10-2026-21:40
    """
    hooks = settings.get("hooks")
    if not isinstance(hooks, dict) or event not in hooks:
        return
    kept = []
    for g in hooks[event]:
        rest = [h for h in g.get("hooks", []) if not is_ours(h.get("command"), "--compact", which)]
        if len(rest) == len(g.get("hooks", [])):
            kept.append(g)
        elif rest:
            kept.append(dict(g, hooks=rest))
    if kept:
        hooks[event] = kept
    else:
        del hooks[event]
    if not hooks:
        del settings["hooks"]


def drop_old_hooks(settings):
    """Takes the hooks of OLD_HOOKS out of `settings`, in place (drop_old_hook).
    @date 28-10-2026-11:35"""
    for event, which in OLD_HOOKS.items():
        drop_old_hook(settings, event, which)


def main(argv):
    """Installs statusline.py as the status line; see the file's description.
    @date 28-10-2026-11:35"""
    return claude_settings.install(argv, "statusline.py", command(), is_ours, drop_old_hooks)


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
