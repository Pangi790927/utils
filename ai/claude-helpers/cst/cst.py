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

How steps 2 and 4 happen depends on where the session runs:
  - Windows: `/exit`, then the launch command with `--resume <id>`, are typed into its console
    (inject.ps1).
  - Linux, a session in a terminal: nothing can type into another terminal there, so the helper
    ends it with SIGTERM, which Claude Code takes as a clean shutdown, and the terminal's shell
    starts it again in the same terminal with `--resume <id>`, through the prompt hook of
    cst.bash (sourced from ~/.bashrc, or by hand in a shell already open). cst refuses to run when
    no shell above the session's claude has that hook, since nothing would bring it back.
  - Linux, a background session (`claude --bg`, the agent view): `claude stop <id>`, then
    `claude --bg --resume <id>`; a terminal that showed it through `claude attach`, in a shell
    with that hook, attaches to it again.

cdt makes the same switch with another rewrite (switch() below). Do not type in the session while
it switches. Everything is logged to cst.log beside this file.

Usage:
    cst [--model M] [--keep 0.5]

@date 24-10-2026-19:25, 27-10-2026-18:40
"""

import argparse
import datetime
import json
import os
import shlex
import signal
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
HELPERS = os.path.dirname(HERE)
sys.path.insert(0, HELPERS)
import ctxtrim      # noqa: E402

IS_WIN = os.name == "nt"
INJECT = os.path.join(HELPERS, "paired", "inject.ps1")
LOG = os.path.join(HERE, "cst.log")
# Linux: the markers of the shells with the cst.bash hook, and the requests left for them; under
# XDG_RUNTIME_DIR, so neither outlives the login. cst.bash picks the same folder. 27-10-2026-18:40
RUN = "" if IS_WIN else os.path.join(os.environ.get("XDG_RUNTIME_DIR") or
                                     f"/tmp/cst-{os.getuid()}", "cst")
DETACHED = 0x00000008 | 0x00000200      # DETACHED_PROCESS | CREATE_NEW_PROCESS_GROUP
QUIET_S = 8                             # seconds without a write before the session counts idle
# The same after a `!` command's output, which Claude Code answers with a turn of its own from
# 2.1.303 on, but an older one may not. 24-10-2026-19:25
BANG_QUIET_S = 60
IDLE_WAIT_S = 1800                      # how long the helper waits for the session to be idle
EXIT_WAIT_S = 60                        # how long it waits for claude to exit once ended
# The hook does not wait for a pending request older than FRESH_S, since the helper that left it
# is gone, and waits READY_WAIT_S at most for the rewrite to finish. 27-10-2026-18:40
FRESH_S = 60
READY_WAIT_S = 180
# What the helper, and every claude it runs, must not inherit from the session cst runs in: its
# identity, and the marks of a command inside a session or a background job.
# 24-10-2026-19:25, 27-10-2026-18:40
INHERITED = ("CLAUDE_CODE", "CLAUDECODE", "CLAUDE_PID", "CLAUDE_EFFORT", "CLAUDE_JOB_DIR")


def log(msg):
    """Appends one time-stamped line to cst.log. @date 10-10-2026"""
    with open(LOG, "a", encoding="utf8") as f:
        f.write(f"{datetime.datetime.now():%m-%d %H:%M:%S} {msg}\n")


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
    if not IS_WIN:
        try:
            return stat_of(pid)[0] != "Z"              # a zombie has exited already
        except (OSError, IndexError):
            return False
    import ctypes
    k = ctypes.windll.kernel32
    h = k.OpenProcess(0x1000, False, int(pid))         # PROCESS_QUERY_LIMITED_INFORMATION
    if not h:
        return False
    code = ctypes.c_ulong()
    ok = k.GetExitCodeProcess(h, ctypes.byref(code))
    k.CloseHandle(h)
    return bool(ok) and code.value == 259              # STILL_ACTIVE


def proc_info(pid):
    """Answers (parent pid, command line) of process `pid`. @date 10-10-2026, Linux 14-10-2026"""
    if not IS_WIN:
        return int(stat_of(pid)[1]), shlex.join(argv_of(pid))
    r = subprocess.run(["powershell", "-NoProfile", "-Command",
                        f"Get-CimInstance Win32_Process -Filter 'ProcessId={int(pid)}' | "
                        f"Select-Object ParentProcessId, CommandLine | ConvertTo-Json"],
                       capture_output=True, text=True, timeout=30)
    o = json.loads(r.stdout)
    return int(o["ParentProcessId"]), o["CommandLine"] or ""


def find_session(sid):
    """Answers the path of session `sid`'s .jsonl, found by its exact id, or None.
    @date 10-10-2026"""
    root = os.path.join(os.path.expanduser("~"), ".claude", "projects")
    for d in os.listdir(root):
        p = os.path.join(root, d, sid + ".jsonl")
        if os.path.exists(p):
            return p
    return None


def inject(pid, text):
    """Types `text` plainly, then Enter, into the console of process `pid` (Windows).
    @date 10-10-2026"""
    tmp = os.path.join(HERE, f"inject.{os.getpid()}.txt")
    with open(tmp, "w", encoding="utf8") as f:
        f.write(text)
    r = subprocess.run(["powershell", "-ExecutionPolicy", "Bypass", "-NoProfile", "-File", INJECT,
                        "-Target", str(pid), "-TextFile", tmp, "-NoPaste",
                        "-LogFile", os.path.join(HERE, "inject.log")],
                       capture_output=True, text=True)
    os.remove(tmp)
    log(f"typed into pid {pid}: {text!r} (exit {r.returncode})")


def run_claude(args, cwd):
    """Runs `claude <args>` in `cwd` and answers its exit code and what it printed, as one line
    for the log. @date 24-10-2026-19:25"""
    try:
        r = subprocess.run(["claude"] + args, cwd=cwd, capture_output=True, text=True,
                           timeout=120)
    except (OSError, subprocess.SubprocessError) as e:
        return f"failed: {e!r}"
    return f"exit {r.returncode}: {' '.join((r.stdout + r.stderr).split())[:300]}"


# --- where a session runs -----------------------------------------------------------------------
# Claude Code on Linux runs a session either in the terminal it was started in, as a child of that
# terminal's shell, or in the background under its daemon (`claude --bg`, the agent view): on a
# terminal of its own, owned by a `claude bg-pty-host` relay, shown only while a terminal runs
# `claude attach <id>`. `claude agents --json` lists both kinds, with the background ones' short
# ids. Seen in 2.1.310. 24-10-2026-19:25

def agent_entry(pid):
    """Answers what `claude agents --json` lists for the session run by process `pid` -- its `kind`
    (interactive or background), short `id`, `status` (idle or busy) and `name` -- or None when it
    is not listed or there is no list. @date 24-10-2026-19:25"""
    try:
        r = subprocess.run(["claude", "agents", "--json"], capture_output=True, text=True,
                           timeout=30)
        return next((s for s in json.loads(r.stdout)
                     if isinstance(s, dict) and s.get("pid") == int(pid)), None)
    except (OSError, ValueError, TypeError, subprocess.SubprocessError):
        return None


def under_relay(pid):
    """Answers whether process `pid` runs on a terminal of Claude Code's daemon, as a background
    session does (Linux). @date 24-10-2026-19:25"""
    try:
        return "bg-pty-host" in proc_info(proc_info(pid)[0])[1]
    except (OSError, IndexError, ValueError):
        return False


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


def resume_command(cmdline, sid):
    """Answers the command that starts session `sid` again, from `cmdline`, the command line its
    claude was started with: see relaunch_args (Windows). @date 10-10-2026, 24-10-2026-19:25"""
    toks = [t.strip('"') for t in shlex.split(cmdline, posix=False)][1:]
    return " ".join(["claude"] + [f'"{t}"' if " " in t else t for t in relaunch_args(toks, sid)])


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


def started(pid):
    """Answers when process `pid` started, in clock ticks since boot, as /proc gives it: with the
    pid, it names that one process and never a later one given the same pid (Linux).
    @date 27-10-2026-18:40"""
    return stat_of(pid)[19]


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
    @date 27-10-2026-18:40"""
    out = []
    for name in os.listdir("/proc"):
        if not name.isdigit():
            continue
        try:
            argv = argv_of(name)
        except OSError:
            continue
        what = argv[2] if len(argv) > 2 else ""
        if os.path.basename(argv[0]) != "claude" or argv[1:2] != ["attach"] or not what:
            continue
        if what == o.bg_id or what.lower() in (o.bg_name or "").lower():
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


def write_json(path, obj):
    """Writes `obj` as JSON to `path`: written whole, then renamed in. @date 27-10-2026-18:40"""
    tmp = path + ".tmp"
    with open(tmp, "w", encoding="utf8") as f:
        json.dump(obj, f)
    os.replace(tmp, path)


def read_request(path):
    """Answers the request in `path`, or None if it is gone or unreadable. @date 14-10-2026"""
    try:
        with open(path, encoding="utf8") as f:
            return json.load(f)
    except (OSError, ValueError):
        return None


def drop_request(path):
    """Removes the request in `path`, if any. @date 14-10-2026, 27-10-2026-18:40"""
    try:
        os.remove(path)
    except OSError:
        pass


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
        r = read_request(os.path.join(RUN, name))
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
        drop_request(path)


def wait_ready(path):
    """Answers the request in `path` once it is "ready", or None when it is gone, or not ready
    within READY_WAIT_S. @date 24-10-2026-19:25"""
    t0 = time.time()
    r = read_request(path)
    while r and r.get("state") != "ready" and time.time() - t0 < READY_WAIT_S:
        time.sleep(0.5)
        r = read_request(path)
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
    r = read_request(path)
    # A request that can't be read never will be; one whose claude still runs, suspended or in
    # the background, waits for that claude to exit. 27-10-2026-18:40
    if not r:
        drop_request(path)
        return None
    if alive(r.get("pid", 0)):
        return None
    try:
        if r.get("state") != "ready" and time.time() - r.get("time", 0) <= FRESH_S:
            r = wait_ready(path) or r
    finally:
        drop_request(path)
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


def is_claude(pid):
    """Answers whether process `pid` still runs claude (Linux). @date 24-10-2026-19:25"""
    try:
        return "claude" in proc_info(pid)[1]
    except (OSError, IndexError, ValueError):
        return False


def end_session(o):
    """Ends the idle session `o` names: types `/exit` into its console (Windows), stops it with
    `claude stop` (Linux, background), or sends its claude SIGTERM (Linux, terminal), leaving the
    pending requests first on Linux. Answers whether claude exited within EXIT_WAIT_S; when it did
    not, the requests are withdrawn. @date 10-10-2026, 24-10-2026-19:25, 27-10-2026-18:40"""
    if IS_WIN:
        inject(o.claude_pid, "/exit")
    else:
        leave_requests(o)
        if o.bg_id:
            log(f"claude stop {o.bg_id}: {run_claude(['stop', o.bg_id], o.cwd)}")
        elif is_claude(o.claude_pid):
            os.kill(o.claude_pid, signal.SIGTERM)
            log(f"SIGTERM to claude {o.claude_pid}")
    t0 = time.time()
    while alive(o.claude_pid):
        if time.time() - t0 > EXIT_WAIT_S:
            if not IS_WIN:
                withdraw_requests(o.sid)
            return False
        time.sleep(0.5)
    return True


def bring_back(o):
    """Brings the ended session `o` names back under its id: types the launch command with
    `--resume <id>` into its console (Windows), or marks its requests ready for the cst.bash hook,
    whose shells start it again in their terminals (Linux), after `claude --bg --resume <id>` for
    a background session. @date 10-10-2026, 24-10-2026-19:25, 27-10-2026-18:40"""
    if IS_WIN:
        inject(o.shell_pid, resume_command(o.cmdline, o.sid))
        return
    if o.bg_id:
        log(f"claude --bg --resume {o.sid}: {run_claude(['--bg', '--resume', o.sid], o.cwd)}")
    mark_ready(o.sid)
    log("requests ready; by hand: " + (f"claude attach {o.bg_id}" if o.bg_id else
                                        f"claude --resume {o.sid}"))


def switch(o, rewrite):
    """@brief Ends the session `o` names once it is idle, rewrites its file, and brings it back.

    Nothing changes when the session closes, is not idle within IDLE_WAIT_S, or its claude does
    not exit. Once claude has exited the session always comes back: rewritten, or as it was when
    the rewrite fails.

    @param o        argparse.Namespace - the helper's options (helper_options)
    @param rewrite  callable(str) -> str - rewrites the session file at the path it is given, in
                    place, and answers a line for the log
    @date 24-10-2026-19:25
    """
    sf = find_session(o.sid)
    t0 = time.time()
    while not is_idle(sf, o.claude_pid):
        if not alive(o.claude_pid) or time.time() - t0 > IDLE_WAIT_S:
            log("session closed or never idle -- nothing changed")
            return
        time.sleep(2)
    if not end_session(o):
        log("claude did not exit -- nothing changed")
        return
    time.sleep(1)
    try:
        log(rewrite(sf))
    except Exception as e:                                              # noqa: BLE001
        log(f"rewrite failed: {e!r} -- it comes back as it was")
    bring_back(o)
    log("done")


# --- starting the helper ------------------------------------------------------------------------

def helper_options(p):
    """Adds to parser `p` the options a helper gets about the session it serves.
    @date 24-10-2026-19:25"""
    p.add_argument("--helper", action="store_true", help=argparse.SUPPRESS)
    for name, kind in (("--sid", str), ("--claude-pid", int), ("--shell-pid", int),
                       ("--cmdline", str), ("--cwd", str), ("--shell", str), ("--bg-id", str),
                       ("--bg-name", str)):
        p.add_argument(name, type=kind, help=argparse.SUPPRESS)


def shown(o):
    """Answers where the session `o` names is shown, for the log. @date 24-10-2026-19:25"""
    if IS_WIN:
        return f"console of pid {o.shell_pid}"
    return f"background session {o.bg_id}" if o.bg_id else f"terminal of shell {o.shell}"


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


def where(cpid):
    """Answers the helper options that tell where the session run by `cpid` is shown and how it
    comes back, or a str saying why it could not come back.
    @date 24-10-2026-19:25, 24-10-2026-20:13, 27-10-2026-18:40"""
    if IS_WIN:
        shell_pid, cmdline = proc_info(cpid)
        return ["--shell-pid", str(shell_pid), "--cmdline", cmdline]
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


def launch_helper(script, sid, cpid, extra):
    """Starts `script --helper` for session `sid`, run by process `cpid`, detached from it, with
    `extra` passed on; answers why it can't, or None once started. @date 24-10-2026-19:25"""
    w = where(cpid)
    if isinstance(w, str):
        return w
    env = {k: v for k, v in os.environ.items() if not k.startswith(INHERITED)}
    detach = {"creationflags": DETACHED} if IS_WIN else {"start_new_session": True}
    subprocess.Popen([sys.executable, os.path.abspath(script), "--helper", "--sid", sid,
                      "--claude-pid", str(cpid)] + w + extra,
                     env=env, close_fds=True, stdin=subprocess.DEVNULL,
                     stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, **detach)
    return None


# --- cst's own: the trim ------------------------------------------------------------------------

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
    trim. @date 18-10-2026-18:25, 24-10-2026-19:25"""
    log(f"=== cst helper: session {o.sid}, claude pid {o.claude_pid}, {shown(o)}")
    made = summarize(o, find_session(o.sid))
    if not made:
        return
    every, split, summary = made

    def trim(sf):
        why = ctxtrim.trim_in_place(sf, every, split, summary)
        return (f"{why} -- nothing changed; it comes back as it was" if why else
                f"trimmed {sf} in place: the summary is record {split}, the records before it "
                f"a log")
    switch(o, trim)


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
    helper_options(p)
    o = p.parse_args(argv)
    if o.take_request:
        return take(o.take_request)
    if o.helper:
        try:
            helper(o)
        except Exception as e:                                          # noqa: BLE001
            log(f"helper failed: {e!r} -- the session file is untouched unless logged above")
        return 0
    here = this_session("cst")
    if not here:
        return 1
    sid, cpid, _ = here
    why = launch_helper(__file__, sid, cpid, ["--model", o.model, "--keep", str(o.keep)])
    if why:
        print(f"cst: {why}")
        return 1
    print(f"cst: trimming session {sid[:8]} in the background (summary by {o.model}); it will end "
          f"and come back by itself in a few minutes. Nothing to do meanwhile; do not type in "
          f"this session until it is back. Log: {LOG}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
