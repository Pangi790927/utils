#!/usr/bin/env python3
"""@file Installs statusline.py into the Claude Code user settings, as the status line, refreshed
every second.

The settings file is ~/.claude/settings.json, or settings.json under CLAUDE_CONFIG_DIR when that
is set. The command written names statusline.py by its absolute path in this checkout, so a PC
that keeps the repo elsewhere works too; after the checkout moves, a new run points it at the new
place. Every other setting is kept, and a run with nothing to change writes nothing. A status
line that is not this one is replaced only with --replace; its other options, such as padding,
are kept. The PreCompact and PostCompact hooks that the install of 27-10-2026-20:40 put in are
taken out. Before a change, the file as it was is copied to settings.json.bak.

Usage:
    install.py [--replace] [--dry]

@date 27-10-2026-20:40, 27-10-2026-21:40
"""

import argparse
import json
import os
import shlex
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
STATUSLINE = os.path.join(HERE, "statusline.py")
SETTINGS = os.path.join(os.environ.get("CLAUDE_CONFIG_DIR") or os.path.expanduser("~/.claude"),
                        "settings.json")
REFRESH_S = 1       # the compaction timer ticks once a second
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


def load(path):
    """Returns the settings in the file at `path`, or {} when there is no such file.

    @param path  str - a settings.json
    @return dict
    @date 27-10-2026-20:40
    """
    if not os.path.exists(path):
        return {}
    with open(path, encoding="utf-8") as f:
        settings = json.load(f)
    if not isinstance(settings, dict):
        sys.exit(f"install: {path} does not hold a JSON object; nothing changed")
    return settings


def set_status_line(settings, replace):
    """Makes statusline.py the status line, refreshed every REFRESH_S seconds.

    It stops the program when another status line is set and `replace` is false.

    @param settings  dict - the settings, changed in place
    @param replace   bool - whether another status line may be replaced
    @date 27-10-2026-20:40
    """
    cur = settings.get("statusLine")
    if isinstance(cur, dict) and is_ours(cur.get("command")):
        new = dict(cur)
    elif cur and not replace:
        sys.exit(f"install: a status line is set already: {json.dumps(cur)}\n"
                 f"run again with --replace to replace it; nothing changed")
    elif isinstance(cur, dict):
        new = dict(cur)
    else:
        new = {}
    new["type"] = "command"
    new["command"] = command()
    new["refreshInterval"] = REFRESH_S
    settings["statusLine"] = new


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


def write(path, settings):
    """Writes `settings` to the file at `path`, keeping the file as it was in path + ".bak".

    The new file replaces the old one in one step, with the same permissions.

    @param path      str - a settings.json
    @param settings  dict - the settings to write
    @date 27-10-2026-20:40
    """
    mode = None
    if os.path.exists(path):
        shutil.copy2(path, path + ".bak")
        mode = os.stat(path).st_mode & 0o777
    os.makedirs(os.path.dirname(path), exist_ok=True)
    tmp = path + ".tmp"
    with open(tmp, "w", encoding="utf-8") as f:
        json.dump(settings, f, indent=2, ensure_ascii=False)
        f.write("\n")
    if mode is not None:
        os.chmod(tmp, mode)
    os.replace(tmp, path)


def main(argv):
    p = argparse.ArgumentParser(description="Installs statusline.py into the Claude Code settings.")
    p.add_argument("--replace", action="store_true",
                   help="replace a status line that is set already")
    p.add_argument("--dry", action="store_true",
                   help="print the settings it would write, and write nothing")
    a = p.parse_args(argv)

    before = load(SETTINGS)
    settings = json.loads(json.dumps(before))
    set_status_line(settings, a.replace)
    for event, which in OLD_HOOKS.items():
        drop_old_hook(settings, event, which)

    if settings == before:
        print(f"install: {SETTINGS} has it all already; nothing changed")
        return 0
    if a.dry:
        print(json.dumps(settings, indent=2, ensure_ascii=False))
        return 0
    write(SETTINGS, settings)
    print(f"install: wrote {SETTINGS}; the file as it was is {SETTINGS}.bak")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
