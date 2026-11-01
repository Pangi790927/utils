#!/usr/bin/env python3
"""@file Installs statusline.sh beside it into the Claude Code user settings, as the status line of
Windows, refreshed every second (statusline/install.py installs the status line of Linux).

The command written runs statusline.sh with bash -- Claude Code runs a status line command in Git
Bash on Windows -- naming it by its absolute path in this checkout, with forward slashes, in
double quotes; after the checkout moves, a new run points it at the new place. The settings file,
--replace and --dry work as for Linux (claude_settings.py). An entry that runs the statusline.py
the Windows install of 28-10-2026-01:10 wrote counts as this one, and is changed without
--replace.

Usage:
    install.py [--replace] [--dry]

@date 28-10-2026-11:35
"""

import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(HERE))
import claude_settings      # noqa: E402

STATUSLINE = os.path.join(HERE, "statusline.sh")
# The commands recognized as this status line, from any checkout. 28-10-2026-11:35
NAMES = ("statusline/windows/statusline.sh", "statusline/statusline.py")


def command():
    """Returns the shell command that runs statusline.sh. @date 28-10-2026-11:35"""
    return 'bash "' + STATUSLINE.replace("\\", "/") + '"'


def is_ours(cmd):
    """Tells whether the command `cmd` runs this status line, of any checkout (NAMES).
    @date 28-10-2026-11:35"""
    return isinstance(cmd, str) and any(name in cmd for name in NAMES)


def main(argv):
    """Installs statusline.sh as the status line; see the file's description.
    @date 28-10-2026-11:35"""
    return claude_settings.install(argv, "statusline/windows/statusline.sh", command(), is_ours)


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
