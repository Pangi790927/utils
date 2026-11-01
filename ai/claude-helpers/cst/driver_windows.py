#!/usr/bin/env python3
"""@file The Windows driver of cst and cdt: how a session is ended, and brought back under its id,
on Windows. common.switch() calls it; driver_linux.py does the same on Linux.

  - A session in a console: `/exit`, then the launch command with `--resume <id>`, are typed into
    its console (inject.ps1). cst refuses to run unless claude was started at a cmd or PowerShell
    prompt that reads the console again once claude exits, since nothing else would take that
    command.
  - A background session (`claude --bg`, the agent view): `claude stop <id>`, then
    `claude --bg --resume <id>`. A console that showed it through `claude attach`, at a cmd or
    PowerShell prompt, gets `claude attach <id>` typed into it once that `claude attach` has
    exited; the agent view lists the session again by itself.
  - The status line of Windows (statusline/windows/statusline.sh) starts no Python for the "back"
    status, so the helper itself removes it at the session's first reply (after_back).

@date 28-10-2026-11:35
"""

import os
import re
import shlex
import subprocess
import time

from common import (EXIT_WAIT_S, HELPERS, HERE, RELAY, agent_entry, attaches_to, clear_status,
                    log, read_json, relaunch_args, replied_since, run_claude, status_path)
from system_windows import alive, proc_info, started, win_processes

INJECT = os.path.join(HELPERS, "paired", "inject.ps1")
# How long the "back" status waits for the first reply of the session that came back;
# statusline/windows/statusline.sh drops an older one, which a helper that died left.
# 28-10-2026-10:45
BACK_SHOW_S = 1800


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


def resume_command(cmdline, sid):
    """Answers the command that starts session `sid` again, from `cmdline`, the command line its
    claude was started with: see relaunch_args (Windows). @date 10-10-2026, 24-10-2026-19:25"""
    toks = [t.strip('"') for t in shlex.split(cmdline, posix=False)][1:]
    return " ".join(["claude"] + [f'"{t}"' if " " in t else t for t in relaunch_args(toks, sid)])


# --- the prompt that brings a session back ------------------------------------------------------
# The helper types the command that brings the session back into the console of the process that
# started claude -- or, for a background session, of each `claude attach` that showed it. Only a
# cmd or PowerShell prompt reads it there once that process has exited: another program, or a
# shell started only to run a script or one command, is gone by then. Git Bash and other shells
# are left out, as untried. 28-10-2026-01:10, 28-10-2026-10:15
SHELLS = ("cmd.exe", "powershell.exe", "pwsh.exe")


def program_of(cmdline):
    """Answers the program a Windows command line starts, as its file name in lower case with
    .exe, and the arguments that follow it. @date 28-10-2026-01:10"""
    m = re.match(r'\s*(?:"([^"]*)"|(\S+))(.*)', cmdline or "", re.S)
    if not m:
        return "", ""
    name = os.path.basename(m.group(1) or m.group(2) or "").lower()
    return (name if name.endswith(".exe") else name + ".exe"), m.group(3)


def runs_once(program, args):
    """Answers whether the shell `program`, started with the arguments `args`, exits once the
    command it was given has run: a cmd with /c, or a PowerShell with -Command, -File or
    -EncodedCommand and no -NoExit ahead of them. @date 28-10-2026-01:10"""
    if program == "cmd.exe":
        m = re.search(r'(?i)(?:^|\s)/([ckr])(?![^\s"])', args)
        return bool(m) and m.group(1).lower() != "k"
    for name in re.findall(r"(?:^|\s)[-/]([A-Za-z]+)", args):
        name = name.lower()
        if len(name) >= 3 and "noexit".startswith(name):
            return False
        if name == "ec" or any(full.startswith(name) for full in
                               ("command", "file", "encodedcommand")):
            return True
    return False


def not_a_prompt(cpid, shell):
    """Answers why process `shell`, which started process `cpid`, would not take a command typed
    into its console once `cpid` exits, or None when it would: it must be one of SHELLS, still the
    very process that started `cpid`, and not one that exits with it (runs_once). A background
    session that `claude agents --json` does not list is told apart by its relay.
    @date 28-10-2026-01:10, 28-10-2026-10:15"""
    try:
        program, args = program_of(proc_info(shell)[1])
        gone = int(started(shell)) > int(started(cpid))
    except (OSError, ValueError, KeyError, TypeError, IndexError, subprocess.SubprocessError):
        gone = True
    if gone:
        return "the program that started this claude is gone"
    if RELAY in args:
        return "this claude runs in the background, but `claude agents --json` does not list it"
    if program not in SHELLS:
        return f"this claude runs under {program}, not at a cmd or PowerShell prompt"
    if runs_once(program, args):
        return (f"the {program} that started this claude exits with it, as it runs only a script "
                f"or one command")
    return None


def prompt_above(pid):
    """Answers the pid of the cmd or PowerShell prompt that started process `pid`, which takes a
    command typed into its console once `pid` exits, or None when `pid` was started some other
    way (not_a_prompt). @date 28-10-2026-10:15"""
    try:
        shell = proc_info(pid)[0]
    except (OSError, ValueError, KeyError, TypeError, IndexError, subprocess.SubprocessError):
        return None
    return None if not_a_prompt(pid, shell) else shell


# --- the consoles that show a background session ------------------------------------------------

def claude_commands():
    """Answers (pid, argv) of each running claude.exe, read from WMI, with the quotes taken off
    each argument. @date 28-10-2026-10:15"""
    out = []
    for p in win_processes("Name='claude.exe'"):
        try:
            argv = [t.strip('"') for t in shlex.split(p["CommandLine"] or "", posix=False)]
        except ValueError:
            continue
        out.append((int(p["ProcessId"]), argv))
    return out


def targets(o):
    """Answers (prompt, pid, arguments) for each `claude attach` that shows the background session
    `o` names, run as process `pid` at a cmd or PowerShell prompt that takes a command typed into
    its console once `pid` exits (prompt_above). @date 28-10-2026-11:35"""
    out = []
    for pid, argv in claude_commands():
        if attaches_to(o, argv):
            shell = prompt_above(pid)
            if shell:
                out.append((shell, pid, argv[1:]))
    return out


def reattach(o):
    """Types `claude attach <id>` at the prompt of each console that showed the background session
    `o` names, once its `claude attach` has exited (o.viewers, from targets); a `claude attach`
    still running after EXIT_WAIT_S is left as it is, and logged. @date 28-10-2026-10:15"""
    for shell, pid, _ in o.viewers:
        t0 = time.time()
        while alive(pid) and time.time() - t0 < EXIT_WAIT_S:
            time.sleep(0.5)
        if alive(pid):
            log(f"claude attach {pid} still runs; nothing typed at prompt {shell}")
        else:
            inject(shell, f"claude attach {o.bg_id}")


# --- ending and bringing back -------------------------------------------------------------------

def end_session(o):
    """Ends the idle session `o` names: stops it with `claude stop` (background), noting first the
    prompts to attach again at (see reattach), or types `/exit` into its console. Answers whether
    claude exited within EXIT_WAIT_S. @date 10-10-2026, 28-10-2026-11:35"""
    if o.bg_id:
        o.viewers = targets(o)
        for shell, pid, _ in o.viewers:
            log(f"claude attach {pid} shows it; prompt {shell} attaches again once it exits")
        log(f"claude stop {o.bg_id}: {run_claude(['stop', o.bg_id], o.cwd)}")
    else:
        inject(o.claude_pid, "/exit")
    t0 = time.time()
    while alive(o.claude_pid):
        if time.time() - t0 > EXIT_WAIT_S:
            return False
        time.sleep(0.5)
    return True


def bring_back(o):
    """Brings the ended session `o` names back under its id: a background session with
    `claude --bg --resume <id>`, after which the consoles that showed it attach again (reattach);
    a session in a console by typing the launch command with `--resume <id>` into it.
    @date 10-10-2026, 28-10-2026-11:35"""
    if o.bg_id:
        log(f"claude --bg --resume {o.sid}: {run_claude(['--bg', '--resume', o.sid], o.cwd)}")
        reattach(o)
        log(f"by hand: claude attach {o.bg_id}")
    else:
        inject(o.shell_pid, resume_command(o.cmdline, o.sid))


def after_back(o, sf):
    """Keeps the "back" status of the session `o` names until the session file `sf` holds a model
    reply newer than it, or for BACK_SHOW_S at most, then removes it. A status that another one
    replaced meanwhile, or that is gone, is left alone. @date 28-10-2026-10:45"""
    since = (read_json(status_path(o.sid)) or {}).get("since")
    if since is None:
        return
    while time.time() - since < BACK_SHOW_S and not replied_since(sf, since):
        time.sleep(2)
        if (read_json(status_path(o.sid)) or {}).get("since") != since:
            return
    clear_status(o.sid)


def shown(o):
    """Answers where the session `o` names is shown, for the log.
    @date 24-10-2026-19:25, 28-10-2026-11:35"""
    return f"background session {o.bg_id}" if o.bg_id else f"console of pid {o.shell_pid}"


def where(cpid, sid):
    """Answers the helper options that tell where session `sid`, run by `cpid`, is shown and how it
    comes back, or a str saying why it could not come back: a background session by its short id
    and folder, as `claude agents --json` lists it; a session in a console by the prompt that
    started its claude (not_a_prompt). @date 24-10-2026-19:25, 28-10-2026-11:35"""
    entry = agent_entry(cpid)
    if (entry or {}).get("kind") == "background" and entry.get("id"):
        return ["--cwd", entry.get("cwd") or os.getcwd(), "--bg-id", entry["id"], "--bg-name",
                entry.get("name") or ""]
    shell_pid, cmdline = proc_info(cpid)
    why = not_a_prompt(cpid, shell_pid)
    if why:
        return (f"{why}, so nothing would bring the session back once it ends. End it, start "
                f"it again with `claude --resume {sid}` at a cmd or PowerShell prompt in the "
                f"same folder, and run this again.")
    return ["--shell-pid", str(shell_pid), "--cmdline", cmdline]
