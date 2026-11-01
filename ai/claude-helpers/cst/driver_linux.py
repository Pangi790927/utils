#!/usr/bin/env python3
"""@file The Linux driver of cst and cdt: how a session is ended, and brought back under its id,
on Linux. common.switch() calls it; driver_windows.py does the same on Windows.

  - A session in a terminal: nothing can type into another terminal there, so the helper ends it
    with SIGTERM, which Claude Code takes as a clean shutdown, and the terminal's shell starts it
    again in the same terminal with `--resume <id>`, through the prompt hook of cst.bash (sourced
    from ~/.bashrc, or by hand in a shell already open). cst refuses to run when no shell above
    the session's claude has that hook, since nothing would bring it back.
  - A background session (`claude --bg`, the agent view): `claude stop <id>`, then
    `claude --bg --resume <id>`; a terminal that showed it through `claude attach`, in a shell
    with that hook, attaches to it again.

@date 24-10-2026-19:25, 27-10-2026-18:40, 28-10-2026-11:35
"""

import os
import shlex
import signal
import sys
import time

from common import (EXIT_WAIT_S, HERE, RELAY, agent_entry, attaches_to, drop_file, log,
                    read_json, relaunch_args, run_claude, write_json)
from system_linux import RUN, alive, argv_of, proc_info, stat_of, started

# The hook does not wait for a pending request older than FRESH_S, since the helper that left it
# is gone, and waits READY_WAIT_S at most for the rewrite to finish. 27-10-2026-18:40
FRESH_S = 60
READY_WAIT_S = 180


def under_relay(pid):
    """Answers whether process `pid` runs on a terminal of Claude Code's daemon, as a background
    session does (Linux). @date 24-10-2026-19:25"""
    try:
        return RELAY in proc_info(proc_info(pid)[0])[1]
    except (OSError, IndexError, ValueError):
        return False


# --- Linux: the shells that bring a session back, and the requests left for them ---------------
# A shell brings claude back once it has sourced cst.bash, which leaves a marker naming the shell
# and hooks its prompt. Before cst or cdt end a session, they leave a request for each shell that
# brings it back, "pending"; they mark it "ready" once the session is rewritten. The hook takes its
# shell's request at the first prompt after the claude the request names has exited.
# 27-10-2026-18:40

def marker_path(shell):
    """Answers the marker file that cst.bash leaves for shell `shell`. @date 27-10-2026-18:40"""
    return os.path.join(RUN, f"shell.{shell}")


def request_path(shell):
    """Answers the file of the request left for shell `shell`.
    @date 14-10-2026, 27-10-2026-18:40"""
    return os.path.join(RUN, f"request.{shell}")


def has_hook(pid):
    """Answers whether process `pid` is a shell that sourced cst.bash: its marker names it by its
    start time, so a marker that a closed shell left never counts. @date 27-10-2026-18:40"""
    try:
        with open(marker_path(pid), encoding="utf8") as f:
            return f.read().strip() == started(pid)
    except (OSError, IndexError):
        return False


def hooked_shell(pid):
    """Answers the pid of the nearest process above process `pid` that is a shell with the
    cst.bash hook, or None. @date 27-10-2026-18:40"""
    try:
        p = int(stat_of(pid)[1])
        while p > 1 and not has_hook(p):
            p = int(stat_of(p)[1])
    except (OSError, IndexError, ValueError):
        return None
    return p if p > 1 else None


def attach_viewers(o):
    """Answers (pid, arguments) of each `claude attach` that shows the background session `o`
    names: one run with its short id, or with part of its name, as `claude attach` takes it.
    @date 27-10-2026-18:40, 28-10-2026-11:35"""
    out = []
    for name in os.listdir("/proc"):
        if not name.isdigit():
            continue
        try:
            argv = argv_of(name)
        except OSError:
            continue
        if os.path.basename(argv[0]) == "claude" and attaches_to(o, argv):
            out.append((int(name), argv[1:]))
    return out


def targets(o):
    """Answers (shell, pid, arguments) for each shell that brings the session `o` names back, once
    process `pid` has exited: the shell above its claude, which runs claude again with the same
    options and `--resume <id>` (relaunch_args); or, for a background session, the shell above
    each `claude attach` that shows it, which runs that `claude attach` again.
    @date 27-10-2026-18:40"""
    if not o.bg_id:
        return [(o.shell, o.claude_pid, relaunch_args(argv_of(o.claude_pid)[1:], o.sid))]
    out = []
    for pid, args in attach_viewers(o):
        shell = hooked_shell(pid)
        if shell:
            out.append((str(shell), pid, args))
    return out


def requests_for(sid):
    """Answers (path, request) for each request left for session `sid`. @date 27-10-2026-18:40"""
    try:
        names = os.listdir(RUN)
    except OSError:
        return []
    out = []
    for name in names:
        if not (name.startswith("request.") and name[len("request."):].isdigit()):
            continue
        r = read_json(os.path.join(RUN, name))
        if r and r.get("sid") == sid:
            out.append((os.path.join(RUN, name), r))
    return out


def leave_requests(o):
    """Leaves a pending request for each shell that brings the session `o` names back (targets),
    and logs them. @date 27-10-2026-18:40"""
    os.makedirs(RUN, exist_ok=True)
    found = targets(o)
    for shell, pid, argv in found:
        write_json(request_path(shell), {"sid": o.sid, "state": "pending", "time": time.time(),
                                         "pid": pid, "argv": argv})
        log(f"request for shell {shell}: claude {shlex.join(argv)}, once {pid} exits")
    if not found:
        log("no shell with the cst.bash hook shows it; it comes back in the background only")


def mark_ready(sid):
    """Marks each request left for session `sid` ready: the session can come back.
    @date 27-10-2026-18:40"""
    for path, r in requests_for(sid):
        write_json(path, dict(r, state="ready"))


def withdraw_requests(sid):
    """Removes each request left for session `sid`. @date 27-10-2026-18:40"""
    for path, _ in requests_for(sid):
        drop_file(path)


def wait_ready(path):
    """Answers the request in `path` once it is "ready", or None when it is gone, or not ready
    within READY_WAIT_S. @date 24-10-2026-19:25"""
    t0 = time.time()
    r = read_json(path)
    while r and r.get("state") != "ready" and time.time() - t0 < READY_WAIT_S:
        time.sleep(0.5)
        r = read_json(path)
    return r if r and r.get("state") == "ready" else None


def take_request(shell):
    """@brief For the cst.bash hook of shell `shell`, at its prompt: answers the arguments to run
    claude with again when cst or cdt ended a claude that shell ran, once that claude has exited
    and the session is rewritten, and consumes the request.

    @param shell  str - the hook's shell, its pid
    @return list[str]|None - None when no request is left for that shell, the claude it names has
            not exited yet, or the session did not come back in time (then it says so on stderr)
    @date 14-10-2026, 24-10-2026-19:25, 27-10-2026-18:40
    """
    path = request_path(shell)
    r = read_json(path)
    # A request that can't be read never will be; one whose claude still runs, suspended or in
    # the background, waits for that claude to exit. 27-10-2026-18:40
    if not r:
        drop_file(path)
        return None
    if alive(r.get("pid", 0)):
        return None
    try:
        if r.get("state") != "ready" and time.time() - r.get("time", 0) <= FRESH_S:
            r = wait_ready(path) or r
    finally:
        drop_file(path)
    if r.get("state") != "ready":
        print(f"cst: session {r.get('sid')} did not come back; bring it back with: claude "
              f"{shlex.join(r.get('argv') or [])}", file=sys.stderr)
        return None
    print(f"cst: bringing back session {r.get('sid')}", file=sys.stderr)
    return r.get("argv") or None


def take(shell):
    """Prints, each followed by a NUL, the arguments the cst.bash hook of shell `shell` runs
    claude with again, and answers 0; answers 1 when there is nothing to bring back.
    @date 24-10-2026-19:25, 27-10-2026-18:40"""
    try:
        again = take_request(shell)
    except KeyboardInterrupt:
        return 1
    if not again:
        return 1
    sys.stdout.write("".join(a + "\0" for a in again))
    return 0


# --- ending and bringing back -------------------------------------------------------------------

def is_claude(pid):
    """Answers whether process `pid` still runs claude (Linux). @date 24-10-2026-19:25"""
    try:
        return "claude" in proc_info(pid)[1]
    except (OSError, IndexError, ValueError):
        return False


def end_session(o):
    """Ends the idle session `o` names: stops it with `claude stop` (background), or sends its
    claude SIGTERM (terminal), leaving the pending requests first. Answers whether claude exited
    within EXIT_WAIT_S; when it did not, the requests are withdrawn.
    @date 10-10-2026, 24-10-2026-19:25, 27-10-2026-18:40, 28-10-2026-11:35"""
    leave_requests(o)
    if o.bg_id:
        log(f"claude stop {o.bg_id}: {run_claude(['stop', o.bg_id], o.cwd)}")
    elif is_claude(o.claude_pid):
        os.kill(o.claude_pid, signal.SIGTERM)
        log(f"SIGTERM to claude {o.claude_pid}")
    t0 = time.time()
    while alive(o.claude_pid):
        if time.time() - t0 > EXIT_WAIT_S:
            withdraw_requests(o.sid)
            return False
        time.sleep(0.5)
    return True


def bring_back(o):
    """Brings the ended session `o` names back under its id: marks its requests ready for the
    cst.bash hook, whose shells start it again in their terminals, after `claude --bg --resume
    <id>` for a background session.
    @date 10-10-2026, 24-10-2026-19:25, 27-10-2026-18:40, 28-10-2026-11:35"""
    if o.bg_id:
        log(f"claude --bg --resume {o.sid}: {run_claude(['--bg', '--resume', o.sid], o.cwd)}")
    mark_ready(o.sid)
    log("requests ready; by hand: " + (f"claude attach {o.bg_id}" if o.bg_id else
                                        f"claude --resume {o.sid}"))


def after_back(o, sf):
    """Does nothing on Linux once the session is back: statusline/statusline.py itself drops the
    "back" status at the session's first reply. @date 28-10-2026-11:35"""


def shown(o):
    """Answers where the session `o` names is shown, for the log.
    @date 24-10-2026-19:25, 28-10-2026-11:35"""
    return f"background session {o.bg_id}" if o.bg_id else f"terminal of shell {o.shell}"


def where(cpid, sid):
    """Answers the helper options that tell where the session run by `cpid` is shown and how it
    comes back, or a str saying why it could not come back; `sid`, the session's id, is not
    needed on Linux. @date 24-10-2026-19:25, 24-10-2026-20:13, 27-10-2026-18:40, 28-10-2026-11:35"""
    out = ["--cwd", os.path.realpath(f"/proc/{cpid}/cwd")]
    entry = agent_entry(cpid)
    if (entry or {}).get("kind") == "background" or under_relay(cpid):
        if not (entry or {}).get("id"):
            return ("it runs in the background, but `claude agents --json` does not list it, so "
                    "it could not be stopped and resumed")
        return out + ["--bg-id", entry["id"], "--bg-name", entry.get("name") or ""]
    shell = hooked_shell(cpid)
    if not shell:
        return (f"no shell above this claude has the cst.bash hook, so nothing would bring the "
                f"session back here. Press Ctrl+Z, run `. {os.path.join(HERE, 'cst.bash')}`, "
                f"then `fg`, and run this again; a new terminal has the hook once ~/.bashrc "
                f"sources cst.bash.")
    return out + ["--shell", str(shell)]
