#!/usr/bin/env python3
"""@file What cst and cdt need from Linux: facts about a process, read from /proc; the folder that
holds the helper's status and the cst.bash markers and requests; and how a program is started.
system_windows.py answers the same on Windows, and common.py picks the one of the system it runs
on.

@date 28-10-2026-11:35
"""

import os
import shlex

# The markers of the shells with the cst.bash hook, the requests left for them, and the helper's
# status: under XDG_RUNTIME_DIR, so none outlives the login. cst.bash picks the same folder.
# 27-10-2026-18:40, 28-10-2026-11:35
RUN = os.path.join(os.environ.get("XDG_RUNTIME_DIR") or f"/tmp/cst-{os.getuid()}", "cst")
# How the helper is started, detached from the session cst runs in; and what a program the helper
# only asks something is started with. 28-10-2026-11:35
DETACH = {"start_new_session": True}
SPAWN = {}
# How a file written whole beside another takes its place. 28-10-2026-11:35
replace = os.replace


def stat_of(pid):
    """Answers the fields /proc gives for process `pid` after its command name, from its state on
    (Linux). @date 27-10-2026-18:40"""
    with open(f"/proc/{int(pid)}/stat", encoding="utf8") as f:
        return f.read().rsplit(")", 1)[1].split()


def argv_of(pid):
    """Answers the command line process `pid` was started with, as a list (Linux).
    @date 27-10-2026-18:40"""
    with open(f"/proc/{int(pid)}/cmdline", "rb") as f:
        return f.read().decode("utf8", "replace").rstrip("\0").split("\0")


def alive(pid):
    """Answers whether process `pid` is still running. @date 10-10-2026, Linux 14-10-2026"""
    try:
        return stat_of(pid)[0] != "Z"              # a zombie has exited already
    except (OSError, IndexError):
        return False


def proc_info(pid):
    """Answers (parent pid, command line) of process `pid`. @date 10-10-2026, Linux 14-10-2026"""
    return int(stat_of(pid)[1]), shlex.join(argv_of(pid))


def started(pid):
    """Answers when process `pid` started, in clock ticks since boot, as /proc gives it: with the
    pid, it names that one process and never a later one given the same pid (Linux).
    @date 27-10-2026-18:40"""
    return stat_of(pid)[19]
