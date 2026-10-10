#!/usr/bin/env python3
"""@file cst -- Claude self-trim: trims the Claude Code session it is run from, in place, and brings
that same session back by itself.

Run it from inside a session: `! cst` (or ask Claude to run it). It identifies the session
exactly, from the CLAUDE_CODE_SESSION_ID and CLAUDE_PID that Claude Code gives its commands, and
refuses to run without them -- it never guesses a session by file times. It then starts a
detached helper and returns at once. The helper:

  1. summarizes the older half of the session (ctxtrim's half-keep: the summarizer sees the whole
     conversation, summarizes only above the split) while the session stays open;
  2. waits until the session is idle, then types `/exit` into its terminal (no focus needed);
  3. once Claude has exited and closed the file, rewrites it in place, atomically:
     [summary of the older half] + [the recent half, verbatim], under the same session id;
  4. types the original launch command, with `--resume <same id>`, into the same terminal.

Do not type in that terminal while it switches. Everything is logged to cst.log beside this file.
No copy of the untrimmed session is kept unless --backup is given (it then goes to backups/).

Usage:
    cst [--model M] [--keep 0.5] [--backup]

@date 10-10-2026
"""

import argparse
import ctypes
import datetime
import json
import os
import shlex
import shutil
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
HELPERS = os.path.dirname(HERE)
sys.path.insert(0, HELPERS)
import ctxtrim      # noqa: E402
import transcript   # noqa: E402

INJECT = os.path.join(HELPERS, "paired", "inject.ps1")
LOG = os.path.join(HERE, "cst.log")
BACKUPS = os.path.join(HERE, "backups")
DETACHED = 0x00000008 | 0x00000200      # DETACHED_PROCESS | CREATE_NEW_PROCESS_GROUP
QUIET_S = 8                             # seconds without a write before the session counts idle


def log(msg):
    """Appends one time-stamped line to cst.log. @date 10-10-2026"""
    with open(LOG, "a", encoding="utf8") as f:
        f.write(f"{datetime.datetime.now():%m-%d %H:%M:%S} {msg}\n")


def alive(pid):
    """Answers whether process `pid` is still running. @date 10-10-2026"""
    k = ctypes.windll.kernel32
    h = k.OpenProcess(0x1000, False, int(pid))         # PROCESS_QUERY_LIMITED_INFORMATION
    if not h:
        return False
    code = ctypes.c_ulong()
    ok = k.GetExitCodeProcess(h, ctypes.byref(code))
    k.CloseHandle(h)
    return bool(ok) and code.value == 259              # STILL_ACTIVE


def proc_info(pid):
    """Answers (parent pid, command line) of process `pid`. @date 10-10-2026"""
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
    """Types `text` plainly, then Enter, into the console of process `pid`. @date 10-10-2026"""
    tmp = os.path.join(HERE, f"inject.{os.getpid()}.txt")
    with open(tmp, "w", encoding="utf8") as f:
        f.write(text)
    r = subprocess.run(["powershell", "-ExecutionPolicy", "Bypass", "-NoProfile", "-File", INJECT,
                        "-Target", str(pid), "-TextFile", tmp, "-NoPaste",
                        "-LogFile", os.path.join(HERE, "inject.log")],
                       capture_output=True, text=True)
    os.remove(tmp)
    log(f"typed into pid {pid}: {text!r} (exit {r.returncode})")


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
    earlier resume, continue or session-id option. @date 10-10-2026"""
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


def helper(o):
    """Does the trim and the switch, detached from the session it serves. @date 10-10-2026"""
    log(f"=== helper: session {o.sid}, claude pid {o.claude_pid}, shell pid {o.shell_pid}")
    sf = find_session(o.sid)
    records = transcript.load_records(sf)
    split = ctxtrim.split_index(records, o.keep)
    summary = ctxtrim.summarize_older(ctxtrim.render_with_marker(records, split, 500), o.model)
    log(f"{len(records)} records, split at {split}; summary {len(summary)} chars")

    t0 = time.time()
    while not is_idle(sf):
        if not alive(o.claude_pid) or time.time() - t0 > 1800:
            log("session closed or never idle -- nothing changed")
            return
        time.sleep(2)
    inject(o.claude_pid, "/exit")
    t0 = time.time()
    while alive(o.claude_pid):
        if time.time() - t0 > 60:
            log("claude did not exit -- nothing changed")
            return
        time.sleep(1)
    time.sleep(1)

    backup = "none"
    if o.backup:
        os.makedirs(BACKUPS, exist_ok=True)
        backup = os.path.join(BACKUPS, f"{o.sid}.{datetime.datetime.now():%Y%m%d-%H%M%S}.jsonl")
        shutil.copy2(sf, backup)
    now = transcript.load_records(sf)
    if [r.get("uuid") for r in now[:split]] != [r.get("uuid") for r in records[:split]]:
        log("the older half changed while summarizing -- nothing changed; resuming as it was")
    else:
        _, new = ctxtrim.build_session(now, split, summary, o.sid)
        tmp = sf + ".cst-tmp"                   # written whole, then swapped in by one rename,
        with open(tmp, "w", encoding="utf8") as f:      # so the session is never half-written
            for r in new:
                f.write(json.dumps(r, ensure_ascii=False) + "\n")
        os.replace(tmp, sf)
        log(f"rewrote {sf}: {len(now)} -> {len(new)} records (backup: {backup})")
    inject(o.shell_pid, resume_command(o.cmdline, o.sid))
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
    p.add_argument("--helper", action="store_true", help=argparse.SUPPRESS)
    p.add_argument("--sid", help=argparse.SUPPRESS)
    p.add_argument("--claude-pid", type=int, help=argparse.SUPPRESS)
    p.add_argument("--shell-pid", type=int, help=argparse.SUPPRESS)
    p.add_argument("--cmdline", help=argparse.SUPPRESS)
    o = p.parse_args(argv)
    if o.helper:
        try:
            helper(o)
        except Exception as e:                                          # noqa: BLE001
            log(f"helper failed: {e!r} -- the session file is untouched unless logged above")
        return 0

    sid, cpid = os.environ.get("CLAUDE_CODE_SESSION_ID"), os.environ.get("CLAUDE_PID")
    if not sid or not cpid:
        print("cst: run it from inside a Claude Code session (`! cst`); "
              "CLAUDE_CODE_SESSION_ID / CLAUDE_PID are not set.")
        return 1
    if not find_session(sid):
        print(f"cst: no session file for {sid}")
        return 1
    shell_pid, cmdline = proc_info(cpid)
    env = {k: v for k, v in os.environ.items()
           if not k.startswith(("CLAUDE_CODE", "CLAUDECODE", "CLAUDE_PID"))}
    subprocess.Popen([sys.executable, os.path.abspath(__file__), "--helper", "--sid", sid,
                      "--claude-pid", cpid, "--shell-pid", str(shell_pid), "--cmdline", cmdline,
                      "--model", o.model, "--keep", str(o.keep)]
                     + (["--backup"] if o.backup else []),
                     env=env, creationflags=DETACHED, close_fds=True,
                     stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL,
                     stderr=subprocess.DEVNULL)
    print(f"cst: trimming session {sid[:8]} in the background (summary by {o.model}). It will "
          f"/exit and resume by itself in a few minutes. Do not type in this terminal until "
          f"it is back. Log: {LOG}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
