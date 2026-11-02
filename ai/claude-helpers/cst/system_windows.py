#!/usr/bin/env python3
"""@file What cst and cdt need from Windows: facts about a process, through the Win32 API and WMI;
the folder that holds the helper's status; and how a program is started. system_linux.py answers
the same on Linux, and common.py picks the one of the system it runs on.

@date 28-10-2026-11:35
"""

import ctypes
import json
import os
import subprocess
import tempfile

import ctxtrim

# The helper's status, for the status line. 28-10-2026-11:35
RUN = os.path.join(os.environ.get("LOCALAPPDATA") or tempfile.gettempdir(), "cst")
# How the helper is started, detached from the session cst runs in (DETACHED_PROCESS |
# CREATE_NEW_PROCESS_GROUP). A program it only asks something runs without a console window of its
# own (CREATE_NO_WINDOW), which would otherwise flash up each time. 28-10-2026-11:35
DETACH = {"creationflags": 0x00000008 | 0x00000200}
SPAWN = {"creationflags": 0x08000000}
# How a file written whole beside another takes its place: Windows refuses the rename for a moment
# while a reader holds the file open, so it is tried again (ctxtrim.replace_file).
# 28-10-2026-11:35
replace = ctxtrim.replace_file


def win_query(pid, ask):
    """Answers what `ask(kernel32, handle)` answers for a handle that queries process `pid`, or
    None when there is no such process. @date 28-10-2026-01:10"""
    k = ctypes.windll.kernel32
    h = k.OpenProcess(0x1000, False, int(pid))         # PROCESS_QUERY_LIMITED_INFORMATION
    if not h:
        return None
    try:
        return ask(k, h)
    finally:
        k.CloseHandle(h)


def win_processes(query):
    """Answers the ProcessId, ParentProcessId and CommandLine of each process the WMI filter
    `query` picks, as a list of dicts. @date 28-10-2026-10:15"""
    quoted = query.replace("'", "''")
    r = subprocess.run(["powershell", "-NoProfile", "-Command",
                        f"ConvertTo-Json -InputObject @(Get-CimInstance Win32_Process -Filter "
                        f"'{quoted}' | Select-Object ProcessId, ParentProcessId, CommandLine)"],
                       capture_output=True, text=True, timeout=30, **SPAWN)
    return json.loads(r.stdout)


def alive(pid):
    """Answers whether process `pid` is still running. @date 10-10-2026, 28-10-2026-11:35"""
    code = ctypes.c_ulong()
    ok = win_query(pid, lambda k, h: k.GetExitCodeProcess(h, ctypes.byref(code)))
    return bool(ok) and code.value == 259              # STILL_ACTIVE


def proc_info(pid):
    """Answers (parent pid, command line) of process `pid`. @date 10-10-2026, 28-10-2026-10:15"""
    o = win_processes(f"ProcessId={int(pid)}")[0]
    return int(o["ParentProcessId"]), o["CommandLine"] or ""


def started(pid):
    """Answers when process `pid` was created, as a string of digits: with the pid, it names that
    one process and never a later one given the same pid. Raises OSError when there is no such
    process. @date 28-10-2026-01:10"""
    times = [ctypes.c_ulonglong() for _ in range(4)]   # creation, exit, kernel and user time
    if not win_query(pid, lambda k, h: k.GetProcessTimes(h, *map(ctypes.byref, times))):
        raise OSError(f"process {pid} can't be queried")
    return str(times[0].value)
