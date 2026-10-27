#!/usr/bin/env python3
"""@file Claude Code status line: the context in use, and what cst or cdt is doing to the session.

As the status line command, it reads the JSON that Claude Code gives a status line on stdin and
prints one colored line:

    60k/1.0M (6%)                          the context in use, as of the last reply
    cst: summarizing… 0:32                 cst or cdt works on this session, at that step so far
    cst: trimmed, waiting for a reply      the session came back, and has not answered since
    cst: nothing changed, see cst.log      cst or cdt stopped; shown for FAILED_SHOW_S

The cst and cdt helper leaves its state in a status file named after the session (cst.py,
set_status); this reads it through cst.py. Once the session that came back has a reply newer than
the status, the status goes, and so does a "failed" one after FAILED_SHOW_S. Linux only, as the
helper's status is. 27-10-2026-21:55

Usage:
    statusline.py    the status line command; install.py puts it into the Claude Code settings

@date 27-10-2026-20:40, 27-10-2026-21:40, 27-10-2026-21:55
"""

import json
import os
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(os.path.dirname(HERE), "cst"))
import cst          # noqa: E402

FAILED_SHOW_S = 60      # how long the status line says that cst or cdt stopped

BLUE = "01;34"
YELLOW = "01;33"
RED = "01;31"
GREY = "01;30"
MAGENTA = "01;35"
CYAN = "01;36"


def helper_state(sid, path):
    """Answers what the status line shows of the cst or cdt helper that serves session `sid`, or
    None when it shows the context instead. A status that has served its time is removed.

    @param sid   str - the session id of the status line input
    @param path  str|None - the transcript_path of the status line input
    @return tuple[str, list]|None - a kind, as state() answers it, and its values
    @date 27-10-2026-21:40, 27-10-2026-21:55
    """
    s = cst.status_of(sid)
    if s is None:
        return None
    what = s.get("what") or "cst"
    text = s.get("text") or ""
    since = s.get("since") or 0
    if s.get("state") == "running":
        return "running", [what, text, int(time.time() - since)]
    if s.get("state") == "back" and not cst.replied_since(path, since):
        return "back", [what, text]
    if s.get("state") == "failed" and time.time() - since < FAILED_SHOW_S:
        return "failed", [what, text]
    cst.clear_status(sid)
    return None


def state(data):
    """Returns what the status line shows, as a kind and its values.

    The kinds are those of helper_state(), ("usage", [percent, tokens, window size]) and
    ("none", []).

    @param data  dict - the status line input
    @return tuple[str, list]
    @date 27-10-2026-20:40, 27-10-2026-21:40
    """
    cw = data.get("context_window") or {}
    pct = cw.get("used_percentage")
    used = cw.get("total_input_tokens")
    size = cw.get("context_window_size")

    sid = data.get("session_id")
    if isinstance(sid, str) and sid and os.path.basename(sid) == sid:
        helper = helper_state(sid, data.get("transcript_path"))
        if helper is not None:
            return helper

    if pct is None or not size:
        return "none", []
    return "usage", [pct, used or 0, size]


def fmt_tokens(n):
    """Returns a token count the short way: 1.2M, 342k, or 512.

    @param n  int - a token count
    @return str
    @date 27-10-2026-20:40
    """
    n = int(n)
    if n >= 1000000:
        return f"{n // 1000000}.{n % 1000000 // 100000}M"
    if n >= 1000:
        return f"{n // 1000}k"
    return str(n)


def fmt_secs(s):
    """Returns a number of seconds as minutes and seconds, as 1:05.

    @param s  int - seconds
    @return str
    @date 27-10-2026-20:40
    """
    return f"{s // 60}:{s % 60:02d}"


def usage_color(pct):
    """Returns the color of a context use: blue under 60%, yellow under 85%, red above.

    @param pct  int - the percentage of the context window in use
    @return str - an SGR color code
    @date 27-10-2026-20:40
    """
    if pct < 60:
        return BLUE
    if pct < 85:
        return YELLOW
    return RED


def render(kind, values):
    """Returns the color and the text of the status line for a state.

    @param kind    str - a kind, as state() returns it
    @param values  list - its values
    @return tuple[str, str]
    @date 27-10-2026-20:40, 27-10-2026-21:40
    """
    if kind == "running":
        what, text, secs = values
        return MAGENTA, f"{what}: {text}… {fmt_secs(secs)}"
    if kind == "back":
        what, text = values
        return CYAN, f"{what}: {text}, waiting for a reply"
    if kind == "failed":
        what, text = values
        return RED, f"{what}: {text}"
    if kind == "usage":
        pct, used, size = values
        return usage_color(pct), f"{fmt_tokens(used)}/{fmt_tokens(size)} ({pct}%)"
    return GREY, "context n/a"


def main():
    """Prints the status line for the status line input JSON on stdin.

    It writes UTF-8 whatever the locale, since the line can hold "…".

    @return int - 0
    @date 27-10-2026-20:40, 27-10-2026-21:40
    """
    try:
        data = json.load(sys.stdin)
    except ValueError:
        data = {}
    if not isinstance(data, dict):
        data = {}
    color, text = render(*state(data))
    sys.stdout.buffer.write(f"\033[{color}m{text}\033[00m".encode("utf-8"))
    return 0


if __name__ == "__main__":
    sys.exit(main())
