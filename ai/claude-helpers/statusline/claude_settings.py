#!/usr/bin/env python3
"""@file The Claude Code user settings, as the status line installers of Linux (install.py) and of
Windows (windows/install.py) change them: where the file is, how it is read and written, and how
an installer sets its status line command in it. Each installer gives its own command, and tells
which commands are its own.

@date 28-10-2026-11:35
"""

import argparse
import json
import os
import shutil
import sys

SETTINGS = os.path.join(os.environ.get("CLAUDE_CONFIG_DIR") or os.path.expanduser("~/.claude"),
                        "settings.json")
REFRESH_S = 1       # the compaction timer ticks once a second


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


def set_status_line(settings, replace, command, is_ours):
    """Makes `command` the status line, refreshed every REFRESH_S seconds.

    It stops the program when another status line is set and `replace` is false.

    @param settings  dict - the settings, changed in place
    @param replace   bool - whether another status line may be replaced
    @param command   str - the status line command
    @param is_ours   callable(str) -> bool - tells whether a command is the installer's own
    @date 27-10-2026-20:40, 28-10-2026-11:35
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
    new["command"] = command
    new["refreshInterval"] = REFRESH_S
    settings["statusLine"] = new


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


def install(argv, what, command, is_ours, tidy=None):
    """@brief Runs an installer: sets its status line in the settings, and writes them.

    `--replace` lets it replace a status line that is not its own, and `--dry` prints the settings
    it would write and writes nothing. A run with nothing to change writes nothing.

    @param argv     list[str] - the installer's arguments
    @param what     str - what it installs, for its --help
    @param command  str - its status line command
    @param is_ours  callable(str) -> bool - tells whether a command is its own
    @param tidy     callable(dict)|None - changes anything else in the settings, in place
    @return int - 0
    @date 27-10-2026-20:40, 28-10-2026-11:35
    """
    p = argparse.ArgumentParser(description=f"Installs {what} into the Claude Code settings.")
    p.add_argument("--replace", action="store_true",
                   help="replace a status line that is set already")
    p.add_argument("--dry", action="store_true",
                   help="print the settings it would write, and write nothing")
    a = p.parse_args(argv)

    before = load(SETTINGS)
    settings = json.loads(json.dumps(before))
    set_status_line(settings, a.replace, command, is_ours)
    if tidy:
        tidy(settings)

    if settings == before:
        print(f"install: {SETTINGS} has it all already; nothing changed")
        return 0
    if a.dry:
        print(json.dumps(settings, indent=2, ensure_ascii=False))
        return 0
    write(SETTINGS, settings)
    print(f"install: wrote {SETTINGS}; the file as it was is {SETTINGS}.bak")
    return 0
