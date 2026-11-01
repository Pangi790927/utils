#!/usr/bin/env python3
"""@file cst -- Claude self-trim: trims the Claude Code session it is run from, in place, and brings
that same session back by itself.

Run it from inside a session: `! cst` (or ask Claude to run it). It identifies the session
exactly, from the CLAUDE_CODE_SESSION_ID and CLAUDE_PID that Claude Code gives its commands, and
refuses to run without them -- it never guesses a session by file times. It then starts a
detached helper and returns at once. The helper:

  1. summarizes the older half of the session (ctxtrim's half-keep: the summarizer sees the whole
     conversation, summarizes only above the split) while the session stays open; only the part
     resume loads counts -- from Claude Code's newest compact boundary, or an earlier trim's
     summary, on;
  2. waits until the session is idle, then ends it;
  3. once Claude has exited, trims the file in place with ctxtrim, under the same session id:
     the summary goes in at the split as the new root, the older half stays in the file as a log
     that resume no longer loads, and the turns written while the summary was made are kept --
     only once it has checked that resume, walking back from the last message, reaches that
     summary;
  4. brings the session back, under the same id, where it was shown.

How steps 2 and 4 happen depends on the system, and its driver does them: driver_linux.py,
through the cst.bash hook of the terminal's shell, and driver_windows.py, by typing into the
session's console; each also stops and resumes a background session. What the two share, the
switch among it, is in common.py.

cdt makes the same switch with another rewrite (common.switch). Do not type in the session while
it switches. Everything is logged to cst.log beside this file. Each step also shows in the
session's status line (common.set_status): statusline/statusline.py on Linux,
statusline/windows/statusline.sh on Windows.

Usage:
    cst [--model M] [--keep 0.5]

@date 24-10-2026-19:25, 27-10-2026-18:40, 27-10-2026-21:55, 28-10-2026-11:35
"""

import argparse
import os
import sys

import common
import ctxtrim      # found through the folder common adds to sys.path. 28-10-2026-11:35
from common import LOG, fail, find_session, log, set_status
# statusline/statusline.py reads the helper's status through these. 28-10-2026-11:35
from common import clear_status, replied_since, status_of      # noqa: F401

driver = common.driver()


def summarize(o, sf):
    """Answers (the session's records, the split, the summary of the half before it) for the
    session in `sf`, or None, logged, when there is nothing to cut or the summary can't replace
    the older half. @date 18-10-2026-18:25, 24-10-2026-19:25"""
    _, every = ctxtrim.load_session(sf)
    base = ctxtrim.live_start(every)            # resume loads from here; before it, logs
    split = base + ctxtrim.split_index(every[base:], o.keep)
    if split >= len(every):
        log("no human prompt to cut at -- nothing changed")
        return None
    text = ctxtrim.render_with_marker(every[base:], split - base, 500)
    summary = ctxtrim.summarize_older(text, o.model)
    log(f"{len(every)} records, loaded from {base} ({len(every) - base}), split at {split}; "
        f"summary {len(summary)} chars")
    bad = ctxtrim.check_summary(summary, len(text.split(ctxtrim.MARKER, 1)[0]))
    if bad:                                     # before the session is touched at all
        log(f"the summary can't replace the older half -- {bad} -- nothing changed")
        return None
    return every, split, summary


def helper(o):
    """Summarizes the older half while the session stays open, then switches the session with the
    trim. @date 18-10-2026-18:25, 24-10-2026-19:25, 27-10-2026-21:40"""
    log(f"=== cst helper: session {o.sid}, claude pid {o.claude_pid}, {driver.shown(o)}")
    set_status(o, "running", "summarizing")
    made = summarize(o, find_session(o.sid))
    if not made:
        fail(o)
        return
    every, split, summary = made

    def trim(sf):
        why = ctxtrim.trim_in_place(sf, every, split, summary)
        if why:
            return False, f"{why} -- nothing changed; it comes back as it was"
        return True, (f"trimmed {sf} in place: the summary is record {split}, the records "
                      f"before it a log")
    common.switch(o, trim, "trimmed", driver)


def main(argv):
    """Starts the detached helper for the session this command runs in; or, for the cst.bash
    function, prints what to run claude with again. @date 10-10-2026, 24-10-2026-19:25"""
    p = argparse.ArgumentParser(description="Trim the Claude Code session you are in, in place, "
                                            "and resume it by itself.")
    p.add_argument("--model", default="claude-opus-5-5", help="model that writes the summary")
    p.add_argument("--keep", type=float, default=0.5,
                   help="share of the conversation kept verbatim")
    p.add_argument("--take-request", metavar="SHELL",
                   help="for the cst.bash hook: print the arguments to run claude with again in "
                        "shell SHELL, once the session cst or cdt ended there is rewritten")
    common.helper_options(p)
    o = p.parse_args(argv)
    if o.take_request:
        return driver.take(o.take_request)
    if o.helper:
        o.what = "cst"
        try:
            helper(o)
        except Exception as e:                                          # noqa: BLE001
            log(f"helper failed: {e!r} -- the session file is untouched unless logged above")
            set_status(o, "failed", f"the helper failed, see {os.path.basename(LOG)}")
        return 0
    here = common.this_session("cst")
    if not here:
        return 1
    sid, cpid, _ = here
    why = common.launch_helper(driver, __file__, sid, cpid,
                               ["--model", o.model, "--keep", str(o.keep)])
    if why:
        print(f"cst: {why}")
        return 1
    print(f"cst: trimming session {sid[:8]} in the background (summary by {o.model}); it will end "
          f"and come back by itself in a few minutes. Nothing to do meanwhile; do not type in "
          f"this session until it is back. Log: {LOG}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
