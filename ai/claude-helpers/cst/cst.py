#!/usr/bin/env python3
"""@file cst -- Claude self-trim: trims the Claude Code session it is run from, in place, and brings
that same session back by itself.

Run it from inside a session: `! cst` (or ask Claude to run it). It identifies the session
exactly, from the CLAUDE_CODE_SESSION_ID and CLAUDE_PID that Claude Code gives its commands, and
refuses to run without them -- it never guesses a session by file times. It then starts a
detached helper and returns at once. The helper:

  1. summarizes the older half of the session (ctxtrim's half-keep: the summarizer sees the whole
     conversation, summarizes only above the split) while the session stays open; only the part
     resume loads counts -- from Claude Code's newest compact boundary on, the rest is dropped;
  2. waits until the session is idle, then types `/exit` into it (no focus needed);
  3. once Claude has exited and closed the file, rewrites it in place, atomically:
     [summary of the older half] + [the recent half, verbatim], under the same session id --
     only once it has checked that resume, walking back from the last message, reaches that
     summary;
  4. brings the session back with `--resume <same id>` in the same terminal.

How steps 2 and 4 happen depends on the system:
  - Windows: `/exit` and then the original launch command are typed into the console (inject.ps1).
  - Linux: Claude Code runs the session under its daemon, on a terminal of its own behind a relay
    socket, and nothing links it to the terminal that shows it. `/exit` is typed through that
    relay (SIGTERM if the relay refuses), and the session comes back through the `claude`
    wrapper of cst.bash (sourced from ~/.bashrc): when its `claude` exits, it asks
    `cst --take-request` whether cst ended a session in its directory just now, and if so starts
    `claude <same arguments> --resume <id>`. Without the wrapper: `claude --resume <id>` by hand.

Do not type in that terminal while it switches. Everything is logged to cst.log beside this file.
No copy of the untrimmed session is kept unless --backup is given (it then goes to backups/).

Usage:
    cst [--model M] [--keep 0.5] [--backup]

@date 10-10-2026, Linux 14-10-2026
"""

import argparse
import datetime
import json
import os
import shlex
import shutil
import signal
import socket
import struct
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
HELPERS = os.path.dirname(HERE)
sys.path.insert(0, HELPERS)
import ctxtrim      # noqa: E402
import transcript   # noqa: E402

IS_WIN = os.name == "nt"
INJECT = os.path.join(HELPERS, "paired", "inject.ps1")
LOG = os.path.join(HERE, "cst.log")
BACKUPS = os.path.join(HERE, "backups")
REQUESTS = os.path.join(HERE, "requests")   # Linux: resume requests for the cst.bash wrapper
DETACHED = 0x00000008 | 0x00000200      # DETACHED_PROCESS | CREATE_NEW_PROCESS_GROUP
QUIET_S = 8                             # seconds without a write before the session counts idle
FRESH_S = 15                            # a request older than this is not for a `claude` exiting now
READY_WAIT_S = 120                      # how long the wrapper waits for a pending trim to finish


def log(msg):
    """Appends one time-stamped line to cst.log. @date 10-10-2026"""
    with open(LOG, "a", encoding="utf8") as f:
        f.write(f"{datetime.datetime.now():%m-%d %H:%M:%S} {msg}\n")


def alive(pid):
    """Answers whether process `pid` is still running. @date 10-10-2026, Linux 14-10-2026"""
    if not IS_WIN:
        try:
            with open(f"/proc/{int(pid)}/stat", encoding="utf8") as f:
                state = f.read().rsplit(")", 1)[1].split()[0]
        except (OSError, IndexError):
            return False
        return state != "Z"                            # a zombie has exited already
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
        with open(f"/proc/{int(pid)}/stat", encoding="utf8") as f:
            ppid = int(f.read().rsplit(")", 1)[1].split()[1])
        with open(f"/proc/{int(pid)}/cmdline", "rb") as f:
            argv = f.read().decode("utf8", "replace").rstrip("\0").split("\0")
        return ppid, shlex.join(argv)
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


# --- Linux: the terminal relay of a session -----------------------------------------------------
# Under Claude Code's daemon a session runs on a terminal of its own, owned by a relay process
# (`claude bg-pty-host --bg-pty-host <socket> ...`, the parent of CLAUDE_PID). Its socket speaks
# frames of [uint32 BE length][uint8 kind][payload]: kind 0 carries raw terminal bytes (what a
# client sends is typed into the session), kind 1 a JSON control message ({"t": "hello"}, "live",
# "ping"/"pong", "auth"/"auth-required", "resize", "kill", "exit"). A new client gets "hello" (with
# the Claude Code version), the screen so far, then "live". Typing is dropped with
# "auth-required" when the relay has a token and the client didn't send it. Seen in Claude Code
# 2.1.295; this is undocumented, so every failure here falls back to SIGTERM. 14-10-2026

RELAY_DATA = 0
RELAY_CTRL = 1


def relay_socket(claude_pid):
    """Answers the relay socket of the session run by `claude_pid`, or None when that process is
    not under a relay (Claude Code started without its daemon). @date 14-10-2026"""
    ppid, _ = proc_info(claude_pid)
    _, cmdline = proc_info(ppid)
    argv = shlex.split(cmdline)
    if "--bg-pty-host" in argv:
        i = argv.index("--bg-pty-host")
        if i + 1 < len(argv) and os.path.exists(argv[i + 1]):
            return argv[i + 1]
    return None


def relay_frame(kind, payload):
    """Answers one relay frame: `payload` is bytes for RELAY_DATA, a dict for RELAY_CTRL.
    @date 14-10-2026"""
    if kind == RELAY_CTRL:
        payload = json.dumps(payload).encode("utf8")
    return struct.pack(">IB", len(payload), kind) + payload


def relay_read(sock, buf, until=None, timeout=5):
    """Reads relay frames until a control message with t == `until` (or the timeout, or the end of
    the connection); answers pings. Returns (the control messages seen, the rest of the buffer).
    @date 14-10-2026"""
    seen = []
    deadline = time.time() + timeout
    while time.time() < deadline:
        while len(buf) >= 5:
            n, kind = struct.unpack(">IB", buf[:5])
            if len(buf) < 5 + n:
                break
            payload, buf = buf[5:5 + n], buf[5 + n:]
            if kind != RELAY_CTRL:
                continue                                # the screen; nothing to do with it
            ctrl = json.loads(payload.decode("utf8", "replace"))
            seen.append(ctrl)
            if ctrl.get("t") == "ping":
                sock.sendall(relay_frame(RELAY_CTRL, {"t": "pong"}))
            if until and ctrl.get("t") == until:
                return seen, buf
        sock.settimeout(max(0.05, deadline - time.time()))
        try:
            chunk = sock.recv(65536)
        except socket.timeout:
            break
        if not chunk:
            break
        buf += chunk
    return seen, buf


def relay_type(path, text):
    """Types `text`, then Enter, into the session behind relay socket `path`, like a terminal
    client does. Answers whether the relay took it. @date 14-10-2026"""
    try:
        with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as s:
            s.connect(path)
            seen, buf = relay_read(s, b"", until="live")
            hello = next((c for c in seen if c.get("t") == "hello"), {})
            if not any(c.get("t") == "live" for c in seen):
                log(f"relay {path}: no 'live' from it (got {[c.get('t') for c in seen]})")
                return False
            s.sendall(relay_frame(RELAY_DATA, text.encode("utf8")))
            time.sleep(0.5)                             # typed, then Enter a moment later
            s.sendall(relay_frame(RELAY_DATA, b"\r"))
            seen, _ = relay_read(s, buf, until="auth-required", timeout=1.5)
            if any(c.get("t") == "auth-required" for c in seen):
                log(f"relay {path}: typing refused (auth-required)")
                return False
            log(f"typed into relay {path} (Claude Code {hello.get('version', '?')}): {text!r}")
            return True
    except (OSError, ValueError) as e:
        log(f"relay {path}: {e!r}")
        return False


# --- Linux: resume requests for the cst.bash wrapper --------------------------------------------

def request_path(sid):
    """Answers the file of the resume request for session `sid`. @date 14-10-2026"""
    return os.path.join(REQUESTS, sid + ".json")


def write_request(sid, cwd, state):
    """Writes the resume request for session `sid`, started in `cwd`: "pending" while cst ends and
    rewrites it, "ready" when it can be resumed. Written whole, then renamed. @date 14-10-2026"""
    os.makedirs(REQUESTS, exist_ok=True)
    tmp = request_path(sid) + ".tmp"
    with open(tmp, "w", encoding="utf8") as f:
        json.dump({"sid": sid, "cwd": cwd, "state": state, "time": time.time()}, f)
    os.replace(tmp, request_path(sid))


def drop_request(sid):
    """Removes the resume request for session `sid`, if any. @date 14-10-2026"""
    try:
        os.remove(request_path(sid))
    except OSError:
        pass


def read_request(path):
    """Answers the request in `path`, or None if it is gone or unreadable. @date 14-10-2026"""
    try:
        with open(path, encoding="utf8") as f:
            return json.load(f)
    except (OSError, ValueError):
        return None


def take_request(cwd):
    """For the cst.bash wrapper, right after its `claude` exited in `cwd`: answers the session id
    to resume when cst ended a session started in `cwd` just now (waiting for its rewrite to
    finish), and consumes that request; None when there is none. @date 14-10-2026"""
    cwd = os.path.realpath(cwd)
    try:
        names = os.listdir(REQUESTS)
    except OSError:
        return None
    for name in names:
        if not name.endswith(".json"):
            continue
        path = os.path.join(REQUESTS, name)
        r = read_request(path)
        if not r or os.path.realpath(r.get("cwd", "")) != cwd:
            continue
        if time.time() - r.get("time", 0) > FRESH_S:
            continue
        t0 = time.time()
        while r and r.get("state") != "ready" and time.time() - t0 < READY_WAIT_S:
            time.sleep(0.5)
            r = read_request(path)
        if not r or r.get("state") != "ready":
            return None
        drop_request(r["sid"])
        return r["sid"]
    return None


# --- the helper ---------------------------------------------------------------------------------

def is_idle(sf):
    """Answers whether the session in `sf` is idle: no write for QUIET_S seconds, and its last
    message is either the model's finished turn or the output of a `!` command. @date 10-10-2026"""
    if time.time() - os.path.getmtime(sf) < QUIET_S:
        return False
    last = None
    for line in open(sf, encoding="utf8", errors="replace"):
        try:
            o = json.loads(line)
        except ValueError:
            continue
        if o.get("type") in ("user", "assistant"):
            last = o
    if not last:
        return False
    m = last.get("message", {})
    if last["type"] == "assistant":
        return m.get("stop_reason") == "end_turn"
    c = m.get("content")
    return isinstance(c, str) and "<bash-stdout>" in c


def resume_command(cmdline, sid):
    """Answers the session's original launch command with `--resume <sid>` in place of any
    earlier resume, continue or session-id option (Windows). @date 10-10-2026"""
    toks = [t.strip('"') for t in shlex.split(cmdline, posix=False)][1:]
    out, skip = [], False
    for t in toks:
        if skip:
            skip = False
            continue
        if t in ("--resume", "-r", "--session-id"):
            skip = True
            continue
        if t in ("--continue", "-c"):
            continue
        out.append(f'"{t}"' if " " in t else t)
    return " ".join(["claude"] + out + ["--resume", sid])


def end_session(o):
    """Makes the idle session exit: types `/exit` into it. On Linux, first leaves a pending resume
    request for the wrapper, and falls back to SIGTERM when the relay doesn't take the typing or
    the session doesn't exit after it. @date 10-10-2026, Linux 14-10-2026"""
    if IS_WIN:
        inject(o.claude_pid, "/exit")
        return
    write_request(o.sid, o.cwd, "pending")
    typed = bool(o.relay) and relay_type(o.relay, "/exit")
    if typed:
        t0 = time.time()
        while alive(o.claude_pid) and time.time() - t0 < 20:
            time.sleep(0.5)
    if alive(o.claude_pid):
        log(f"{'/exit did not end it' if typed else 'no relay typing'} -- SIGTERM to {o.claude_pid}")
        try:
            os.kill(o.claude_pid, signal.SIGTERM)
        except OSError as e:
            log(f"SIGTERM failed: {e!r}")


def bring_back(o):
    """Resumes the session in its terminal: types the launch command (Windows), or marks the
    request ready for the cst.bash wrapper (Linux). @date 10-10-2026, Linux 14-10-2026"""
    if IS_WIN:
        inject(o.shell_pid, resume_command(o.cmdline, o.sid))
        return
    write_request(o.sid, o.cwd, "ready")
    log(f"resume request ready for the wrapper in {o.cwd} (by hand: claude --resume {o.sid})")


def helper(o):
    """Does the trim and the switch, detached from the session it serves. @date 10-10-2026"""
    log(f"=== helper: session {o.sid}, claude pid {o.claude_pid}, "
        + (f"shell pid {o.shell_pid}" if IS_WIN else f"relay {o.relay}, cwd {o.cwd}"))
    sf = find_session(o.sid)
    every = transcript.load_records(sf)
    base = ctxtrim.live_start(every)            # resume loads from here; before it is dead history
    records = every[base:]
    split = ctxtrim.split_index(records, o.keep)
    text = ctxtrim.render_with_marker(records, split, 500)
    summary = ctxtrim.summarize_older(text, o.model)
    log(f"{len(every)} records, loaded from {base} ({len(records)}), split at {base + split}; "
        f"summary {len(summary)} chars")
    bad = ctxtrim.check_summary(summary, len(text.split(ctxtrim.MARKER, 1)[0]))
    if bad:                                     # before the session is touched at all
        log(f"the summary can't replace the older half -- {bad} -- nothing changed")
        return

    t0 = time.time()
    while not is_idle(sf):
        if not alive(o.claude_pid) or time.time() - t0 > 1800:
            log("session closed or never idle -- nothing changed")
            return
        time.sleep(2)
    end_session(o)
    t0 = time.time()
    while alive(o.claude_pid):
        if time.time() - t0 > 60:
            log("claude did not exit -- nothing changed")
            if not IS_WIN:
                drop_request(o.sid)
            return
        time.sleep(1)
    time.sleep(1)

    backup = "none"
    if o.backup:
        os.makedirs(BACKUPS, exist_ok=True)
        backup = os.path.join(BACKUPS, f"{o.sid}.{datetime.datetime.now():%Y%m%d-%H%M%S}.jsonl")
        shutil.copy2(sf, backup)
    now = transcript.load_records(sf)
    new = None
    if (ctxtrim.live_start(now) != base or [r.get("uuid") for r in now[base:base + split]]
            != [r.get("uuid") for r in records[:split]]):
        log("the older half changed while summarizing -- nothing changed; resuming as it was")
    else:
        # nothing is deleted: the history before `base` and the older half stay in the file, ahead
        # of the summary, where resume no longer reaches them (14-10-2026)
        _, trimmed = ctxtrim.build_session(now[base:], split, summary, o.sid, keep_older=True)
        new = now[:base] + trimmed
        if ctxtrim.chain_root(new) != base + split:     # resume must start at the summary
            log("trim not effective: resume would not reach the summary -- nothing changed")
            new = None
    if new:
        tmp = sf + ".cst-tmp"                   # written whole, then swapped in by one rename,
        with open(tmp, "w", encoding="utf8") as f:      # so the session is never half-written
            for r in new:
                f.write(json.dumps(r, ensure_ascii=False) + "\n")
        os.replace(tmp, sf)
        loaded = len(ctxtrim.loaded_uuids(new[base + split:]))
        log(f"rewrote {sf}: resume now loads {loaded} records from the summary on; "
            f"the {split} older records stay in the file, unreached (backup: {backup})")
    bring_back(o)
    log("done")


def main(argv):
    """Starts the detached helper for the session this command runs in. @date 10-10-2026"""
    p = argparse.ArgumentParser(description="Trim the Claude Code session you are in, in place, "
                                            "and resume it by itself.")
    p.add_argument("--model", default="claude-opus-5-5", help="model that writes the summary")
    p.add_argument("--keep", type=float, default=0.5,
                   help="share of the conversation kept verbatim")
    p.add_argument("--backup", action="store_true",
                   help="keep a copy of the untrimmed session in backups/ (off by default)")
    p.add_argument("--take-request", metavar="CWD",
                   help="for the cst.bash wrapper: print the session to resume in CWD, if any")
    p.add_argument("--helper", action="store_true", help=argparse.SUPPRESS)
    p.add_argument("--sid", help=argparse.SUPPRESS)
    p.add_argument("--claude-pid", type=int, help=argparse.SUPPRESS)
    p.add_argument("--shell-pid", type=int, help=argparse.SUPPRESS)
    p.add_argument("--cmdline", help=argparse.SUPPRESS)
    p.add_argument("--relay", help=argparse.SUPPRESS)
    p.add_argument("--cwd", help=argparse.SUPPRESS)
    o = p.parse_args(argv)
    if o.take_request:
        sid = take_request(o.take_request)
        if not sid:
            return 1
        print(sid)
        return 0
    if o.helper:
        try:
            helper(o)
        except Exception as e:                                          # noqa: BLE001
            log(f"helper failed: {e!r} -- the session file is untouched unless logged above")
            if not IS_WIN:
                drop_request(o.sid)
        return 0

    sid, cpid = os.environ.get("CLAUDE_CODE_SESSION_ID"), os.environ.get("CLAUDE_PID")
    if not sid or not cpid:
        print("cst: run it from inside a Claude Code session (`! cst`); "
              "CLAUDE_CODE_SESSION_ID / CLAUDE_PID are not set.")
        return 1
    if not find_session(sid):
        print(f"cst: no session file for {sid}")
        return 1
    if IS_WIN:
        shell_pid, cmdline = proc_info(cpid)
        where = ["--shell-pid", str(shell_pid), "--cmdline", cmdline]
        detach = {"creationflags": DETACHED}
        back = "resume by itself"
    else:
        relay = relay_socket(cpid)
        where = ["--cwd", os.path.realpath(f"/proc/{int(cpid)}/cwd")]
        where += ["--relay", relay] if relay else []
        detach = {"start_new_session": True}
        back = (f"come back by itself through the cst.bash wrapper (without it: "
                f"claude --resume {sid})")
    env = {k: v for k, v in os.environ.items()
           if not k.startswith(("CLAUDE_CODE", "CLAUDECODE", "CLAUDE_PID"))}
    subprocess.Popen([sys.executable, os.path.abspath(__file__), "--helper", "--sid", sid,
                      "--claude-pid", cpid, "--model", o.model, "--keep", str(o.keep)]
                     + where + (["--backup"] if o.backup else []),
                     env=env, close_fds=True, stdin=subprocess.DEVNULL,
                     stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, **detach)
    print(f"cst: trimming session {sid[:8]} in the background (summary by {o.model}). It will "
          f"/exit and {back} in a few minutes. Do not type in this terminal until it is back. "
          f"Log: {LOG}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
