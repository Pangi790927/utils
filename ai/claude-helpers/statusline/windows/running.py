#!/usr/bin/env python3
"""@file The running part of the status line of Windows: what the cst or cdt helper is doing to the
session, while it does it.

statusline.sh beside it runs this with the status line input JSON on stdin, only while the status
the helper left for the session says "running", and shows what it prints:

    cst: summarizing… 0:32                 cst or cdt works on this session, at that step so far

It prints nothing when the helper that left the status has exited -- the status is removed then
(common.status_of) -- or when the status says something else; statusline.sh shows the context
instead. Every other line comes from statusline.sh, which starts no Python for them: starting
Python every second made typing in Claude Code freeze now and then.

Usage:
    running.py    run by statusline.sh

@date 28-10-2026-10:45, 28-10-2026-11:35
"""

import json
import os
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(os.path.dirname(os.path.dirname(HERE)), "cst"))
import common       # noqa: E402

MAGENTA = "01;35"


def fmt_secs(s):
    """Returns a number of seconds as minutes and seconds, as 1:05.

    @param s  int - seconds
    @return str
    @date 27-10-2026-20:40
    """
    return f"{s // 60}:{s % 60:02d}"


def running(data):
    """Returns the line that shows the running helper of the session the status line input names,
    with how long it has been at its step, or "" when no helper runs for it.

    @param data  dict - the status line input
    @return str - the line, with its color, or ""
    @date 28-10-2026-10:45, 28-10-2026-11:35
    """
    sid = data.get("session_id")
    if not (isinstance(sid, str) and sid and os.path.basename(sid) == sid):
        return ""
    s = common.status_of(sid)
    if not s or s.get("state") != "running":
        return ""
    secs = int(time.time() - (s.get("since") or 0))
    text = f"{s.get('what') or 'cst'}: {s.get('text') or ''}… {fmt_secs(secs)}"
    return f"\033[{MAGENTA}m{text}\033[00m"


def main():
    """Prints the line of the running helper for the status line input JSON on stdin, if any.

    It writes UTF-8 whatever the locale, since the line holds "…".

    @return int - 0
    @date 27-10-2026-20:40, 28-10-2026-10:45
    """
    try:
        data = json.load(sys.stdin)
    except ValueError:
        data = {}
    sys.stdout.buffer.write(running(data if isinstance(data, dict) else {}).encode("utf-8"))
    return 0


if __name__ == "__main__":
    sys.exit(main())
