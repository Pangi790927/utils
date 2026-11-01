#!/usr/bin/env python3
"""@file What cst and cdt share on Linux and on Windows alike: the log, a session's file and its
last message, when a session counts as idle, the status the status line shows, how the detached
helper is started, and the switch -- ending an idle session, rewriting its file, and bringing it
back.

Nothing here behaves differently on the two systems. Ending a session and bringing it back do,
and the driver of the system does them (driver_linux.py, driver_windows.py; driver() picks it).
What this needs to know about processes, and the folder of the status files, comes from
system_linux.py or system_windows.py.

@date 28-10-2026-11:35
"""

import argparse
import datetime
import importlib
import json
import os
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
HELPERS = os.path.dirname(HERE)
sys.path.insert(0, HELPERS)
import ctxtrim      # noqa: E402

if os.name == "nt":
    import system_windows as system     # noqa: E402
else:
    import system_linux as system       # noqa: E402

LOG = os.path.join(HERE, "cst.log")
QUIET_S = 8                             # seconds without a write before the session counts idle
# The same after a `!` command's output, which Claude Code answers with a turn of its own from
# 2.1.303 on, but an older one may not. 24-10-2026-19:25
BANG_QUIET_S = 60
IDLE_WAIT_S = 1800                      # how long the helper waits for the session to be idle
EXIT_WAIT_S = 60                        # how long it waits for claude to exit once ended
# What the helper, and every claude it runs, must not inherit from the session cst runs in: its
# identity, and the marks of a command inside a session or a background job.
# 24-10-2026-19:25, 27-10-2026-18:40
INHERITED = ("CLAUDE_CODE", "CLAUDECODE", "CLAUDE_PID", "CLAUDE_EFFORT", "CLAUDE_JOB_DIR")


def driver():
    """Answers the driver of the system this runs on: driver_windows on Windows, driver_linux
    elsewhere. @date 28-10-2026-11:35"""
    return importlib.import_module("driver_windows" if os.name == "nt" else "driver_linux")


def log(msg):
    """Appends one time-stamped line to cst.log. @date 10-10-2026"""
    with open(LOG, "a", encoding="utf8") as f:
        f.write(f"{datetime.datetime.now():%m-%d %H:%M:%S} {msg}\n")


def find_session(sid):
    """Answers the path of session `sid`'s .jsonl, found by its exact id, or None.
    @date 10-10-2026"""
    root = os.path.join(os.path.expanduser("~"), ".claude", "projects")
    for d in os.listdir(root):
        p = os.path.join(root, d, sid + ".jsonl")
        if os.path.exists(p):
            return p
    return None


def run_claude(args, cwd):
    """Runs `claude <args>` in `cwd` and answers its exit code and what it printed, as one line
    for the log. @date 24-10-2026-19:25"""
    try:
        r = subprocess.run(["claude"] + args, cwd=cwd, capture_output=True, text=True,
                           timeout=120, **system.SPAWN)
    except (OSError, subprocess.SubprocessError) as e:
        return f"failed: {e!r}"
    return f"exit {r.returncode}: {' '.join((r.stdout + r.stderr).split())[:300]}"


# --- where a session runs -----------------------------------------------------------------------
# Claude Code runs a session either in the terminal it was started in, as a child of that
# terminal's shell, or in the background under its daemon (`claude --bg`, the agent view): on a
# terminal of its own, owned by a `claude bg-pty-host` relay (RELAY), shown only while a terminal
# runs `claude attach <id>`. `claude agents --json` lists both kinds, with the background ones'
# short ids. Seen in 2.1.310 on Linux, and in 2.1.311 on Windows. 24-10-2026-19:25,
# 28-10-2026-11:35
RELAY = "bg-pty-host"


def agent_entry(pid):
    """Answers what `claude agents --json` lists for the session run by process `pid` -- its `kind`
    (interactive or background), short `id`, `status` (idle or busy) and `name` -- or None when it
    is not listed or there is no list. @date 24-10-2026-19:25"""
    try:
        r = subprocess.run(["claude", "agents", "--json"], capture_output=True, text=True,
                           timeout=30, **system.SPAWN)
        return next((s for s in json.loads(r.stdout)
                     if isinstance(s, dict) and s.get("pid") == int(pid)), None)
    except (OSError, ValueError, TypeError, subprocess.SubprocessError):
        return None


def attaches_to(o, argv):
    """Answers whether the command line `argv` is a `claude attach` that shows the background
    session `o` names: one run with its short id, or with part of its name, as `claude attach`
    takes it. @date 28-10-2026-11:35"""
    what = argv[2] if len(argv) > 2 else ""
    return (argv[1:2] == ["attach"] and bool(what)
            and (what == o.bg_id or what.lower() in (o.bg_name or "").lower()))


# --- how a session is started again -------------------------------------------------------------
# How `claude` reads its options, from 2.1.310's --help: these take no value, these take every
# value up to the next option, and any other takes one, as --model does. They tell an option's
# value from a prompt given on the command line, which the session must not get again when it
# comes back. A relaunch leaves out NOT_AGAIN: it resumes the session itself, interactively.
# 24-10-2026-19:25
NO_VALUE = {
    "--allow-dangerously-skip-permissions", "--ax-screen-reader", "--bg", "--background", "--bare",
    "--brief", "--chrome", "-c", "--continue", "--dangerously-skip-permissions", "--desktop",
    "--disable-slash-commands", "--exclude-dynamic-system-prompt-sections", "--fork-session",
    "--forward-subagent-text", "-h", "--help", "--ide", "--include-hook-events",
    "--include-partial-messages", "--no-chrome", "--no-session-persistence", "-p", "--print",
    "--replay-user-messages", "--restricted", "--safe-mode", "--strict-mcp-config", "--tmux",
    "--verbose", "-v", "--version"}
MANY_VALUES = {"--add-dir", "--allowedTools", "--allowed-tools", "--betas", "--disallowedTools",
               "--disallowed-tools", "--file", "--mcp-config", "--tools"}
NOT_AGAIN = {"-r", "--resume", "-c", "--continue", "--session-id", "--fork-session", "-p",
             "--print", "--bg", "--background"}


def relaunch_args(args, sid):
    """Answers the arguments that start session `sid` again the way `args` started claude: the
    same options, without a prompt given on the command line or an option that picks, forks or
    backgrounds a session, and with `--resume <sid>`. @date 24-10-2026-19:25"""
    out, i = [], 0
    while i < len(args):
        a, i = args[i], i + 1
        if a == "--":
            break                                       # the rest is a prompt. 24-10-2026-19:25
        if not a.startswith("-") or a == "-":
            continue                                    # a prompt. 24-10-2026-19:25
        name = a.split("=", 1)[0]
        most = 0 if name in NO_VALUE or "=" in a else len(args) if name in MANY_VALUES else 1
        j = i
        while j < len(args) and j - i < most and not args[j].startswith("-"):
            j += 1
        if name not in NOT_AGAIN:
            out += args[i - 1:j]
        i = j
    return out + ["--resume", sid]


# --- files the helper leaves ----------------------------------------------------------------------

def write_json(path, obj):
    """Writes `obj` as JSON to `path`: written whole, then renamed in (system.replace).
    @date 27-10-2026-18:40, 28-10-2026-11:35"""
    tmp = path + ".tmp"
    with open(tmp, "w", encoding="utf8") as f:
        json.dump(obj, f)
    system.replace(tmp, path)


def read_json(path):
    """Answers the JSON object in `path`, a request or a status, or None if it is gone or
    unreadable. @date 14-10-2026, 27-10-2026-21:40"""
    try:
        with open(path, encoding="utf8") as f:
            return json.load(f)
    except (OSError, ValueError):
        return None


def drop_file(path):
    """Removes the file at `path`, a request or a status, if any.
    @date 14-10-2026, 27-10-2026-18:40, 27-10-2026-21:40"""
    try:
        os.remove(path)
    except OSError:
        pass


# --- what the helper is doing, for the status line ----------------------------------------------
# The helper leaves its state in a status file named after the session, and the status line of
# the system shows it in that session's status line: "running" while it summarizes or waits for
# the session to be idle, "back" once the session comes back, until its first reply
# (replied_since), and "failed" when nothing changed. 27-10-2026-21:40, 27-10-2026-21:55,
# 28-10-2026-11:35

def status_path(sid):
    """Answers the status file of the helper that serves session `sid`. @date 27-10-2026-21:40"""
    return os.path.join(system.RUN, f"status.{sid}")


def set_status(o, state, text):
    """Leaves for the status line the state of the helper that serves the session `o` names:
    `state` is "running", "back" or "failed", and `text` says what it does or what came of it. A
    status that can't be written is only logged: the switch goes on without it.
    @date 27-10-2026-21:40, 28-10-2026-11:35"""
    me = os.getpid()
    try:
        os.makedirs(system.RUN, exist_ok=True)
        write_json(status_path(o.sid), {"what": o.what, "state": state, "text": text,
                                        "since": time.time(), "pid": me,
                                        "started": system.started(me)})
    except (OSError, IndexError) as e:
        log(f"status not written: {e!r}")


def status_of(sid):
    """@brief Answers the state the helper that serves session `sid` left, for the status line.

    A "running" status whose helper has exited is removed, and answers None.

    @param sid  str - a session id
    @return dict|None - "what" ("cst" or "cdt"), "state", "text", and "since", when that state
            began, in Unix seconds; None when there is none
    @date 27-10-2026-21:40
    """
    path = status_path(sid)
    s = read_json(path)
    if not isinstance(s, dict):
        return None
    if s.get("state") == "running":
        try:
            mine = (system.alive(s.get("pid", 0))
                    and system.started(s.get("pid", 0)) == s.get("started"))
        except (OSError, IndexError):
            mine = False
        if not mine:
            drop_file(path)
            return None
    return s


def clear_status(sid):
    """Removes the status left for session `sid`, if any. @date 27-10-2026-21:40"""
    drop_file(status_path(sid))


def replied_since(sf, since):
    """Answers whether the session in `sf` holds a model reply written after `since`, in Unix
    seconds: the last message is the model's, and newer. @date 27-10-2026-21:55"""
    try:
        last = last_message(sf) or {}
    except (OSError, TypeError):
        return False
    t = ctxtrim.when(last)
    return last.get("type") == "assistant" and t is not None and t.timestamp() > since


# --- the switch ---------------------------------------------------------------------------------

def last_message(sf):
    """Answers the last user or assistant record of the session file `sf`, or None.
    @date 24-10-2026-19:25"""
    with open(sf, "rb") as f:
        end = f.seek(0, os.SEEK_END)
        size = 1 << 16
        while True:
            start = max(0, end - size)
            f.seek(start)
            lines = f.read(end - start).split(b"\n")[1 if start else 0:]
            for line in reversed(lines):
                try:
                    r = json.loads(line)
                except ValueError:
                    continue
                if isinstance(r, dict) and r.get("type") in ("user", "assistant"):
                    return r
            if not start:
                return None
            size *= 4


def is_idle(sf, pid):
    """Answers whether the session in `sf`, run by process `pid`, is idle: nothing written to its
    file for QUIET_S seconds, its last message a finished answer of the model -- or the output of
    a `!` command, after BANG_QUIET_S, for a Claude Code that answers none -- and its status idle
    where `claude agents --json` lists it. @date 10-10-2026, 24-10-2026-19:25"""
    quiet = time.time() - os.path.getmtime(sf)
    if quiet < QUIET_S:
        return False
    last = last_message(sf) or {}
    m = last.get("message") or {}
    if last.get("type") == "assistant":
        if m.get("stop_reason") != "end_turn":
            return False
    elif not (isinstance(m.get("content"), str) and "<bash-stdout>" in m["content"]
              and quiet >= BANG_QUIET_S):
        return False
    entry = agent_entry(pid)
    return entry is None or entry.get("status") == "idle"


def fail(o, why=None):
    """Says in the status line that the helper for the session `o` names stopped with nothing
    changed, and logs `why` unless it was logged already. @date 27-10-2026-21:40"""
    if why:
        log(f"{why} -- nothing changed")
    set_status(o, "failed", f"nothing changed, see {os.path.basename(LOG)}")


def switch(o, rewrite, done, drv):
    """@brief Ends the session `o` names once it is idle, rewrites its file, and brings it back.

    Nothing changes when the session closes, is not idle within IDLE_WAIT_S, or its claude does
    not exit. Once claude has exited the session always comes back: rewritten, or as it was when
    the rewrite fails. Each step shows in the session's status line (set_status).

    @param o        argparse.Namespace - the helper's options (helper_options), with `what`, the
                    tool's name, "cst" or "cdt"
    @param rewrite  callable(str) -> (bool, str) - rewrites the session file at the path it is
                    given, in place, and answers whether it changed the file, and a line for the
                    log
    @param done     str - what the status line says of a rewrite that changed the file
    @param drv      module - the driver of the system (driver()): its end_session(o) ends the
                    session, its bring_back(o) brings it back, and its after_back(o, sf) does
                    what the system needs once it is back
    @date 24-10-2026-19:25, 27-10-2026-21:40, 28-10-2026-11:35
    """
    sf = find_session(o.sid)
    set_status(o, "running", "waiting for the session to be idle")
    t0 = time.time()
    while not is_idle(sf, o.claude_pid):
        if not system.alive(o.claude_pid) or time.time() - t0 > IDLE_WAIT_S:
            fail(o, "session closed or never idle")
            return
        time.sleep(2)
    set_status(o, "running", "ending the session")
    if not drv.end_session(o):
        fail(o, "claude did not exit")
        return
    time.sleep(1)
    changed = False
    try:
        changed, line = rewrite(sf)
        log(line)
    except Exception as e:                                              # noqa: BLE001
        log(f"rewrite failed: {e!r} -- it comes back as it was")
    set_status(o, "back", done if changed else f"unchanged, see {os.path.basename(LOG)}")
    drv.bring_back(o)
    log("done")
    drv.after_back(o, sf)


# --- starting the helper ------------------------------------------------------------------------

def helper_options(p):
    """Adds to parser `p` the options a helper gets about the session it serves.
    @date 24-10-2026-19:25"""
    p.add_argument("--helper", action="store_true", help=argparse.SUPPRESS)
    for name, kind in (("--sid", str), ("--claude-pid", int), ("--shell-pid", int),
                       ("--cmdline", str), ("--cwd", str), ("--shell", str), ("--bg-id", str),
                       ("--bg-name", str)):
        p.add_argument(name, type=kind, help=argparse.SUPPRESS)


def this_session(name):
    """Answers (id, claude pid, session file) of the session this command runs in; when there is
    none, prints why, headed by `name`, and answers None. @date 24-10-2026-19:25"""
    sid, cpid = os.environ.get("CLAUDE_CODE_SESSION_ID"), os.environ.get("CLAUDE_PID")
    if not sid or not cpid:
        print(f"{name}: run it from inside a Claude Code session (`! {name}`); "
              f"CLAUDE_CODE_SESSION_ID / CLAUDE_PID are not set.")
        return None
    path = find_session(sid)
    if not path:
        print(f"{name}: no session file for {sid}")
        return None
    return sid, int(cpid), path


def launch_helper(drv, script, sid, cpid, extra):
    """Starts `script --helper` for session `sid`, run by process `cpid`, detached from it, with
    `extra` passed on and the options `drv.where` gives; answers why it can't, or None once
    started. @date 24-10-2026-19:25, 28-10-2026-11:35"""
    w = drv.where(cpid, sid)
    if isinstance(w, str):
        return w
    env = {k: v for k, v in os.environ.items() if not k.startswith(INHERITED)}
    subprocess.Popen([sys.executable, os.path.abspath(script), "--helper", "--sid", sid,
                      "--claude-pid", str(cpid)] + w + extra,
                     env=env, close_fds=True, stdin=subprocess.DEVNULL,
                     stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, **system.DETACH)
    return None
