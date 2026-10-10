#!/usr/bin/env python3
"""@file Orchestrates the two-model console-RTS loop, owning the live agent windows itself.

Each agent is a real interactive session in its own console window (opened by this orchestrator
with CREATE_NEW_CONSOLE, so it can relaunch one in place). Turns are relayed by typing into an
agent with inject.ps1 (WriteConsoleInput) and reading the reply from that session's .jsonl.

Because the orchestrator owns the windows it does two things the old split could not:

  - HALF-KEEP SHRINK: when an agent's context reaches the configured share of its window, it is
    stopped, ctxtrim.py
    trims it (keeping the recent tail verbatim, summarizing the older part), and the agent is
    relaunched with `claude --resume <trimmed-sid>` -- so OUR summarize governs the shrink, not
    the host's whole-conversation compaction.
  - STOP / CONTINUE: on exit the live session ids are saved to <dir>/.sessions; `--resume` relaunches
    both agents on those ids and carries on where they left off.

All settings come from config.json beside run.ps1: per agent the model, effort, tools and role
(appended to its system prompt, so the role survives every trim), plus the shared rules written
to game/CLAUDE.md, rounds, idle and trim. GLM is product owner / QA, Claude is the implementer.
Writes converse.log, glm.live/cld.live, game.live, glm/cld.context.txt, loop.ndjson, logs/, and
commits the game per round.

@date 08-10-2026-13:30
"""

import argparse
import datetime
import glob
import json
import os
import re
import shutil
import subprocess
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import transcript  # noqa: E402

HELP_DIR = os.path.dirname(os.path.abspath(__file__))
CTXTRIM = os.path.join(HELP_DIR, "ctxtrim.py")
TRANSCRIPT = os.path.join(HELP_DIR, "transcript.py")
GAME_MD = os.path.join(HELP_DIR, "game.md")
UTILS = "C:/Users/apangratie/workspace/utils"
NEW_CONSOLE = getattr(subprocess, "CREATE_NEW_CONSOLE", 0x00000010)

# The run's settings live in config.json next to run.ps1: model, effort, tools and role per agent,
# the shared rules (game/CLAUDE.md), rounds, idle and trim. Lines starting with // are comments.
# 09-10-2026
AGENT_KEY = {"impl": "implementer", "glm": "qa"}


def load_config(path):
    """Answers the settings in `path`, a JSON file whose lines starting with // are comments.
    @date 09-10-2026"""
    with open(path, encoding="utf8") as f:
        cfg = json.loads("".join(ln for ln in f if not ln.lstrip().startswith("//")))
    cfg["_path"] = path
    return cfg


def write_shared_rules(d, cfg):
    """Writes the config's shared rules to <d>/CLAUDE.md, which both agents read.
    @date 09-10-2026"""
    with open(os.path.join(d, "CLAUDE.md"), "w", encoding="utf8") as f:
        f.write(lines_text(cfg["shared_rules"]) + "\n")


def reload_config(cfg, d, files):
    """Answers config.json as it is now, so edits made during a run are picked up: the loop
    settings from the next round, and an agent's model, effort, tools and role at its next
    (re)launch -- a trim or a recovery. A file that does not parse (say, half-saved) is reported
    and the current settings are kept. @date 09-10-2026"""
    try:
        new = load_config(cfg["_path"])
    except (OSError, ValueError) as e:
        emit(files, "~~~ ", f"config.json does not parse ({e}); keeping the current settings")
        return cfg
    if new != cfg:
        emit(files, "~~~ ", "config.json changed: loop settings apply now; model, effort, tools "
                            "and role apply at each agent's next trim or relaunch")
        if new.get("shared_rules") != cfg.get("shared_rules"):
            write_shared_rules(d, new)
    return new


def lines_text(v):
    """Answers a config text, which may be one string or a list of lines. @date 09-10-2026"""
    return "\n".join(v) if isinstance(v, list) else str(v)


def agent_cfg(cfg, role):
    """Answers the config block of agent `role` ("impl" or "glm"). @date 09-10-2026"""
    return cfg[AGENT_KEY[role]]



# -------------------------------------------------------------------------------------------------
# Sessions: find, read turns
# -------------------------------------------------------------------------------------------------

def find_session(sid):
    """Answers the path of session `sid`'s .jsonl under the projects dir, or None. @date 08-10"""
    hits = glob.glob(os.path.join(os.path.expanduser("~"), ".claude", "projects", "*",
                                  sid + ".jsonl"))
    return hits[0] if hits else None


def end_turns(sid):
    """Answers (count, last_text) of completed assistant turns in session `sid`. @date 08-10"""
    sf = find_session(sid)
    if not sf:
        return 0, ""
    cnt, last = 0, ""
    try:
        for line in open(sf, encoding="utf8", errors="replace"):
            try:
                o = json.loads(line)
            except ValueError:
                continue
            m = o.get("message", {}) if isinstance(o.get("message"), dict) else {}
            if o.get("type") == "assistant" and m.get("stop_reason") == "end_turn":
                t = "\n".join(b.get("text", "") for b in m.get("content", [])
                              if isinstance(b, dict) and b.get("type") == "text").strip()
                if t:
                    cnt += 1
                    last = t
    except OSError:
        pass
    return cnt, last


def session_bytes(sid):
    """Answers the byte size of session `sid`'s file, or 0. @date 08-10"""
    sf = find_session(sid)
    return os.path.getsize(sf) if sf and os.path.exists(sf) else 0




def context_tokens(sid):
    """Answers the agent's current context size in tokens, read from the latest assistant turn's
    usage (prompt cache + input + output), or 0. @date 08-10-2026-14:00"""
    sf = find_session(sid)
    if not sf:
        return 0
    usage = None
    try:
        for line in open(sf, encoding="utf8", errors="replace"):
            try:
                o = json.loads(line)
            except ValueError:
                continue
            if o.get("type") == "assistant":
                u = o.get("message", {}).get("usage")
                if isinstance(u, dict):
                    usage = u
    except OSError:
        return 0
    if not usage:
        return 0
    return sum(int(usage.get(k, 0) or 0) for k in ("input_tokens", "cache_creation_input_tokens",
                                                   "cache_read_input_tokens", "output_tokens"))


# -------------------------------------------------------------------------------------------------
# Agent processes (owned here, in their own console windows)
# -------------------------------------------------------------------------------------------------

def child_pid(ppid):
    """Answers a claude/node child pid of `ppid` (the injectable process for a cmd-hosted GLM), or
    `ppid` itself. @date 08-10"""
    try:
        r = subprocess.run(["powershell", "-NoProfile", "-Command",
                            f"(Get-CimInstance Win32_Process -Filter 'ParentProcessId={ppid}' | "
                            f"Where-Object {{$_.Name -match 'claude|node'}} | "
                            f"Select-Object -First 1).ProcessId"],
                           capture_output=True, text=True, timeout=20)
        out = (r.stdout or "").strip()
        return int(out) if out.isdigit() else ppid
    except Exception:                                               # noqa: BLE001
        return ppid


def launch(role, sid, resume, game_dir, coop, cfg):
    """Opens agent `role` in its own console window, configured from `cfg` (model, effort, tools,
    and its role appended to the system prompt, so the role survives every trim), and answers
    (proc, inject_pid). `resume` continues session `sid`; otherwise it is created. The window pid
    is recorded in <coop>/<role>.winpid so run.ps1 can tear it down. @date 09-10-2026"""
    a = agent_cfg(cfg, role)
    role_file = os.path.join(coop, f"{role}.role.md")
    with open(role_file, "w", encoding="utf8") as f:
        f.write(lines_text(a["role"]))
    cmd = ["claude"] if role == "impl" else ["cmd", "/c", "claude-glm.cmd"]
    cmd += (["--resume", sid] if resume else ["--session-id", sid])
    if a.get("model"):
        cmd += ["--model", a["model"]]
    if a.get("effort"):
        cmd += ["--effort", a["effort"]]
    cmd += ["--append-system-prompt-file", role_file, "--allowedTools"] + list(a["tools"])
    proc = subprocess.Popen(cmd, cwd=game_dir, creationflags=NEW_CONSOLE)
    try:
        with open(os.path.join(coop, f"{role}.winpid"), "w", encoding="utf8") as f:
            f.write(str(proc.pid))
    except OSError:
        pass
    time.sleep(6)
    pid = proc.pid if role == "impl" else child_pid(proc.pid)
    return proc, pid


def kill(proc):
    """Ends an agent process tree. @date 08-10"""
    try:
        subprocess.run(["taskkill", "/PID", str(proc.pid), "/T", "/F"], capture_output=True)
    except Exception:                                               # noqa: BLE001
        pass


# -------------------------------------------------------------------------------------------------
# Talking to an agent window
# -------------------------------------------------------------------------------------------------

def inject(inject_ps1, pid, text, coop, agent):
    """Types `text` into agent window `pid`'s console via inject.ps1. @date 08-10"""
    tmp = os.path.join(coop, f"{agent}.in")
    with open(tmp, "w", encoding="utf8") as f:
        f.write(text)
    subprocess.run(["powershell", "-ExecutionPolicy", "Bypass", "-NoProfile", "-File", inject_ps1,
                    "-Target", str(pid), "-TextFile", tmp], capture_output=True, text=True)


def answer_trust(inject_ps1, pid):
    """Answers Claude Code's "trust this folder?" dialog in agent window `pid` with Yes when it is
    on screen, and presses nothing when it is not. Blind keys are unsafe here: one version puts
    "No, exit" first and preselected. @date 10-10-2026"""
    subprocess.run(["powershell", "-ExecutionPolicy", "Bypass", "-NoProfile", "-File", inject_ps1,
                    "-Target", str(pid), "-Trust"], capture_output=True, text=True)


# Typing a message longer than about 4 KB into the agent's console loses its beginning, so a
# longer one is handed over as a file and only a one-line pointer is typed. 09-10-2026
TYPE_LIMIT = 3500


def deliverable(text, coop, agent):
    """Answers what to type for `text`: the text itself when short; otherwise a one-line pointer
    to <coop>/<agent>.msg.md, where the full message is written with its line breaks intact.
    @date 09-10-2026"""
    if len(text) <= TYPE_LIMIT:
        return text
    with open(os.path.join(coop, f"{agent}.msg.md"), "w", encoding="utf8") as f:
        f.write(text)
    rel = f"{os.path.basename(coop)}/{agent}.msg.md"
    return (f"This turn's message is long, so it is in the file {rel} in the current directory. "
            f"Read all of it now and act on it exactly as if it had been typed here.")


def ask(inject_ps1, pid, sid, text, coop, agent, idle):
    """Injects a turn and waits for the session's next completed reply, for as long as the agent
    keeps working. It re-sends a lone Enter (then re-pastes) while the message seems undelivered,
    and gives up only when the session has written nothing for `idle` seconds -- an inactive
    agent, not a slow one. Answers (reply|None, seconds). @date 09-10-2026"""
    text = deliverable(text, coop, agent)
    base, _ = end_turns(sid)
    base_size = last_size = session_bytes(sid)
    inject(inject_ps1, pid, text, coop, agent)
    t0 = last = last_growth = time.time()
    retries = 0
    while True:
        time.sleep(1.5)
        now = time.time()
        cnt, reply = end_turns(sid)
        if cnt > base:
            return reply, now - t0
        size = session_bytes(sid)
        if size != last_size:
            last_size, last_growth = size, now
        if size <= base_size and now - last > 12 and retries < 4:
            inject(inject_ps1, pid, "" if retries < 2 else text, coop, agent)
            last = last_growth = now
            retries += 1
        elif now - last_growth > idle:
            return None, now - t0


NUDGE = ("Your last turn stopped without finishing (the connection dropped or the machine slept). "
         "Continue where you left off and finish the turn with your report.")


def ask_recovering(agent, proc, pid, sid, text, inj, coop, d, cfg, files):
    """Asks an agent for its turn, recovering it only when it goes inactive: first a "continue"
    nudge, then a relaunch on the same session with the turn re-sent. Answers (reply|None,
    seconds, proc, pid) -- None only when the agent stayed inactive through both. @date 09-10-2026"""
    idle = cfg["idle_seconds"]
    out, dur = ask(inj, pid, sid, text, coop, agent, idle)
    if out is not None:
        return out, dur, proc, pid
    emit(files, "~~~ ", f"{agent} inactive {idle}s -- nudging it to continue")
    out, more = ask(inj, pid, sid, NUDGE, coop, agent, idle)
    if out is not None:
        return out, dur + more, proc, pid
    emit(files, "~~~ ", f"{agent} still inactive -- relaunching it on session {sid[:8]}")
    kill(proc)
    time.sleep(2)
    proc, pid = launch(agent, sid, True, d, coop, cfg)
    out, last = ask(inj, pid, sid, text, coop, agent, idle)
    return out, dur + more + last, proc, pid


# ES_CONTINUOUS | ES_SYSTEM_REQUIRED: the PC stays awake while the loop runs; the screen may still
# turn off. 09-10-2026
_AWAKE = 0x80000000 | 0x00000001


def keep_awake(on):
    """Asks Windows to keep the machine from sleeping while `on`, and releases that when off.
    @date 09-10-2026"""
    try:
        import ctypes
        ctypes.windll.kernel32.SetThreadExecutionState(_AWAKE if on else 0x80000000)
    except Exception:                                               # noqa: BLE001
        pass


# -------------------------------------------------------------------------------------------------
# Views, logging, git
# -------------------------------------------------------------------------------------------------

def emit(files, prefix, text):
    """Writes one turn wrapped under `prefix` to stdout, the combined log and the speaker stream."""
    block = "\n".join(transcript.wrap_prose(prefix, text)) + "\n\n"
    sys.stdout.write(block)
    sys.stdout.flush()
    files["convo"].write(block)
    files["convo"].flush()
    s = files.get("glm") if prefix == "glm>" else files.get("cld") if prefix == "cld>" else None
    if s:
        s.write(block)
        s.flush()


def log_turn(path, rnd, role, sid, rc, dur, out):
    rec = {"ts": datetime.datetime.now().isoformat(timespec="seconds"), "round": rnd, "role": role,
           "sid": sid, "session_bytes": session_bytes(sid), "rc": rc, "dur_s": round(dur, 1),
           "stdout": out}
    with open(path, "a", encoding="utf8") as fh:
        fh.write(json.dumps(rec, ensure_ascii=False) + "\n")


def archive_previous(game_dir):
    names = ("converse.log", "glm.live", "cld.live", "game.live", "glm.context.txt",
             "cld.context.txt")
    have = [os.path.join(game_dir, n) for n in names
            if os.path.exists(os.path.join(game_dir, n))
            and os.path.getsize(os.path.join(game_dir, n)) > 0]
    if not have:
        return
    dest = os.path.join(game_dir, "logs", datetime.datetime.now().strftime("%Y%m%d-%H%M%S"))
    os.makedirs(dest, exist_ok=True)
    for p in have:
        try:
            shutil.move(p, os.path.join(dest, os.path.basename(p)))
        except OSError:
            pass


def render_context(sid, out_path):
    """Writes the agent's FULL context: tool inputs and results in full (no cap) and the injected
    context blocks raw, not the `///` id summaries. @date 08-10-2026-14:00"""
    sf = find_session(sid)
    if sf:
        subprocess.run([sys.executable, TRANSCRIPT, sf, "-o", out_path, "--full-attachments",
                        "--truncate", "0"],
                       capture_output=True, text=True, encoding="utf8", errors="replace")


def game_view(game_dir, live_path, rnd):
    exe = os.path.join(game_dir, "main.exe")
    with open(live_path, "a", encoding="utf8") as g:
        g.write(f"\n===== game `main.exe --test` after round {rnd} =====\n")
        if not os.path.exists(exe):
            g.write("(main.exe not built yet)\n")
            return
        try:
            r = subprocess.run([exe, "--test"], cwd=game_dir, capture_output=True, text=True,
                               encoding="utf8", errors="replace", timeout=120)
            g.write((r.stdout or "") + (r.stderr or "") + f"\n[exit {r.returncode}]\n")
        except Exception as e:                                      # noqa: BLE001
            g.write(f"(could not run: {e})\n")


def git(d, *a):
    return subprocess.run(["git", "-C", d, *a], capture_output=True, text=True, encoding="utf8",
                          errors="replace")


def git_setup(d):
    if os.path.isdir(os.path.join(d, ".git")):
        return
    git(d, "init")
    git(d, "config", "user.name", "gamedev-loop")
    git(d, "config", "user.email", "gamedev-loop@localhost")
    with open(os.path.join(d, ".gitignore"), "w", encoding="utf8") as f:
        f.write("\n".join(["*.exe", "*.obj", "*.pdb", "*.ilk", "*.exp", "*.lib", "vc140.pdb",
                           "*.live", "*.context.txt", "loop.ndjson", "converse.log", "logfile*.log",
                           "test_logfile*.log", ".coop/", "logs/", ".sessions"]) + "\n")
    git(d, "add", "-A")
    git(d, "commit", "-m", "gamedev loop: initial state")


def git_commit(d, msg):
    git(d, "add", "-A")
    git(d, "commit", "-m", msg)


# -------------------------------------------------------------------------------------------------
# The half-keep shrink, and session save/restore
# -------------------------------------------------------------------------------------------------

# The owner's drop-boxes in the game directory: one read by both agents, one per agent. 09-10-2026
OWNER_NOTES = {"both": "owner_note.txt", "glm": "owner_note.qa.txt",
               "impl": "owner_note.implementer.txt"}


def take_owner_note(d, name):
    """Answers the owner's note from <d>/<name>, or "" when the file is empty. A note is
    delivered once: it is copied into logs/ and the file is left in place, emptied, ready for the
    next one. The file is created empty when missing. @date 09-10-2026"""
    p = os.path.join(d, name)
    if not os.path.exists(p):
        open(p, "w", encoding="utf8").close()
        return ""
    text = open(p, encoding="utf8", errors="replace").read().strip()
    if not text:
        return ""
    dest = os.path.join(d, "logs")
    os.makedirs(dest, exist_ok=True)
    stamp = datetime.datetime.now().strftime("%Y%m%d-%H%M%S")
    stem = name[:-len(".txt")]
    with open(os.path.join(dest, f"{stem}.{stamp}.txt"), "w", encoding="utf8") as f:
        f.write(text + "\n")
    open(p, "w", encoding="utf8").close()
    return text


def owner_preamble(d, agent, both, files):
    """Answers the owner's message for `agent` this round -- the shared note `both` plus the
    agent's own note -- marked as coming from the owner, or "" when there is none.
    @date 09-10-2026"""
    own = take_owner_note(d, OWNER_NOTES[agent])
    if own:
        emit(files, "~~~ ", f"owner note delivered to {agent}: {own[:200]}")
    text = "\n\n".join(t for t in (both, own) if t)
    return f"***MESSAGE FROM PANGI/ANDREI (the owner): {text}***\n\n" if text else ""


def save_sessions(d, impl_sid, glm_sid):
    with open(os.path.join(d, ".sessions"), "w", encoding="utf8") as f:
        f.write(f"impl={impl_sid}\nglm={glm_sid}\n")


def read_sessions(d):
    p = os.path.join(d, ".sessions")
    out = {}
    if os.path.exists(p):
        for line in open(p, encoding="utf8"):
            if "=" in line:
                k, v = line.strip().split("=", 1)
                out[k] = v
    return out.get("impl"), out.get("glm")


# After a trim fails, wait this long before trying that agent again. 09-10-2026
TRIM_RETRY_S = 1800
_trim_after = {}


def trim_if_big(agent, proc, pid, sid, game_dir, coop, files, cfg):
    """When session `sid` reaches cfg trim.at of the agent's context window, run ctxtrim (keep
    trim.keep_recent verbatim, summarize the older part) and, only when it produced a usable
    summary, stop the agent and relaunch it resuming the trimmed session. A failed trim leaves the
    agent running untouched and is retried after TRIM_RETRY_S. The summary is written by the SAME
    bot whose session it is: the implementer's by its model through claude, GLM's by GLM through
    claude-glm.cmd. Answers (proc, pid, sid), updated on a trim. @date 09-10-2026"""
    a = agent_cfg(cfg, agent)
    window, at, keep = a["context_window"], cfg["trim"]["at"], cfg["trim"]["keep_recent"]
    toks = context_tokens(sid)
    if toks < at * window or time.time() < _trim_after.get(agent, 0):
        return proc, pid, sid
    sf = find_session(sid)
    emit(files, "~~~ ", f"{agent} at {toks} tokens (~{toks / window:.0%} of {window}) -- "
                        f"half-keep trim, summarized by {agent} itself")
    who = (["--cli", "glm"] if agent == "glm" else
           ["--cli", "claude", "--model", a.get("model") or "fable"])
    r = subprocess.run([sys.executable, CTXTRIM, sf, "--out-dir", os.path.dirname(sf),
                        "--keep", str(keep)] + who,
                       capture_output=True, text=True, encoding="utf8", errors="replace")
    m = re.search(r"resume ([0-9a-f-]{36})", r.stderr or "")
    if not m:
        _trim_after[agent] = time.time() + TRIM_RETRY_S
        last = (r.stderr or "").strip().splitlines()[-1:] or ["no output"]
        emit(files, "~~~ ", f"trim of {agent} failed ({last[0][:200]}); {agent} keeps running "
                            f"untouched, next try in {TRIM_RETRY_S // 60} min")
        return proc, pid, sid
    new_sid = m.group(1)
    kill(proc)
    time.sleep(2)
    proc, pid = launch(agent, new_sid, True, game_dir, coop, cfg)
    emit(files, "~~~ ", f"{agent} resumed on trimmed session {new_sid}")
    return proc, pid, new_sid

# -------------------------------------------------------------------------------------------------
# Entry
# -------------------------------------------------------------------------------------------------

def main(argv):
    p = argparse.ArgumentParser(description="Own and relay the two live 3D-RTS agents.")
    p.add_argument("--dir", required=True, help="the game directory")
    p.add_argument("--config", help="settings file (default: config.json beside the game dir)")
    opt = p.parse_args(argv)
    d = opt.dir
    cfg = load_config(opt.config or os.path.join(os.path.dirname(os.path.abspath(d)),
                                                 "config.json"))
    os.makedirs(d, exist_ok=True)
    archive_previous(d)
    if os.path.exists(GAME_MD) and not os.path.exists(os.path.join(d, "game.md")):
        shutil.copy(GAME_MD, os.path.join(d, "game.md"))   # seed only; the agents own it after
    write_shared_rules(d, cfg)
    coop = os.path.join(d, ".coop")
    os.makedirs(coop, exist_ok=True)
    inj = os.path.join(os.path.dirname(d), "inject.ps1")
    live = {"convo": os.path.join(d, "converse.log"), "glm": os.path.join(d, "glm.live"),
            "cld": os.path.join(d, "cld.live")}
    game_live = os.path.join(d, "game.live")
    glm_ctx, cld_ctx = os.path.join(d, "glm.context.txt"), os.path.join(d, "cld.context.txt")
    ndjson = os.path.join(d, "loop.ndjson")
    for pth in list(live.values()) + [game_live, glm_ctx, cld_ctx]:
        open(pth, "w", encoding="utf8").close()
    files = {k: open(v, "a", encoding="utf8") for k, v in live.items()}
    git_setup(d)

    def when():
        return datetime.datetime.now().strftime("%H:%M:%S")

    resuming = not cfg.get("fresh_start", False)
    if resuming:
        impl_sid, glm_sid = read_sessions(d)
        resuming = bool(impl_sid and glm_sid)
    if not resuming:
        import uuid
        impl_sid, glm_sid = str(uuid.uuid4()), str(uuid.uuid4())

    impl_proc = glm_proc = None
    keep_awake(True)
    try:
        emit(files, "~~~ ", f"{'resuming' if resuming else 'starting'} agents in {d} at {when()}")
        glm_proc, glm_pid = launch("glm", glm_sid, resuming, d, coop, cfg)
        impl_proc, impl_pid = launch("impl", impl_sid, resuming, d, coop, cfg)
        save_sessions(d, impl_sid, glm_sid)
        time.sleep(8)
        for pid in (glm_pid, impl_pid):                 # a first-run "trust this folder?"
            answer_trust(inj, pid)                      # dialog, answered Yes only if shown

        rounds = int(cfg.get("rounds", 0))
        emit(files, "~~~ ", f"loop: {str(rounds) + ' rounds' if rounds > 0 else 'unbounded'}")
        impl_last = ""
        r = 0
        while True:
            cfg = reload_config(cfg, d, files)
            rounds = int(cfg.get("rounds", 0))
            if 0 < rounds <= r:
                break
            r += 1
            if glm_proc.poll() is not None or impl_proc.poll() is not None:
                emit(files, "~~~ ", "an agent window was closed -- stopping")
                break
            both = take_owner_note(d, OWNER_NOTES["both"])
            if both:
                emit(files, "~~~ ", f"owner note delivered to both agents: {both[:200]}")
            # Each agent's role sits in its system prompt (config.json), so a turn carries only
            # the relay: the partner's last message and what to do with it. 09-10-2026
            qa_msg = (owner_preamble(d, "glm", both, files)
                      + (f"Claude just reported:\n\n{impl_last}\n\n" if impl_last else "")
                      + "Build with `make`, run `./main.exe --test`, look at a "
                      "`./main.exe --screenshot` when visuals matter, then give your next "
                      "ideas and requests.")
            out, dur, glm_proc, glm_pid = ask_recovering("glm", glm_proc, glm_pid, glm_sid, qa_msg,
                                                         inj, coop, d, cfg, files)
            log_turn(ndjson, r, "glm", glm_sid, 0 if out else -1, dur, out or "")
            if out is None:
                emit(files, "~~~ ", "GLM stayed inactive after a nudge and a relaunch -- stopping")
                break
            emit(files, "glm>", f"[round {r} @ {when()}, {dur:.0f}s]\n{out}")
            qa_last = out
            render_context(glm_sid, glm_ctx)
            glm_proc, glm_pid, glm_sid = trim_if_big("glm", glm_proc, glm_pid, glm_sid, d, coop,
                                                     files, cfg)
            save_sessions(d, impl_sid, glm_sid)

            impl_msg = (owner_preamble(d, "impl", both, files)
                        + f"GLM's ideas and requests:\n\n{qa_last}\n\nImplement them, keep "
                        f"`make` green and `./main.exe --test` passing, then report.")
            out, dur, impl_proc, impl_pid = ask_recovering("impl", impl_proc, impl_pid, impl_sid,
                                                           impl_msg, inj, coop, d, cfg, files)
            log_turn(ndjson, r, "cld", impl_sid, 0 if out else -1, dur, out or "")
            if out is None:
                emit(files, "~~~ ", "Claude stayed inactive after a nudge and a relaunch -- stopping")
                break
            emit(files, "cld>", f"[round {r} @ {when()}, {dur:.0f}s]\n{out}")
            impl_last = out
            render_context(impl_sid, cld_ctx)
            game_view(d, game_live, r)
            git_commit(d, f"round {r}: " + (impl_last.splitlines()[0][:72] if impl_last else ""))
            impl_proc, impl_pid, impl_sid = trim_if_big("impl", impl_proc, impl_pid, impl_sid, d,
                                                        coop, files, cfg)
            save_sessions(d, impl_sid, glm_sid)

        emit(files, "~~~ ", f"loop end {when()}")
    finally:
        keep_awake(False)
        save_sessions(d, impl_sid, glm_sid)
        for pr in (impl_proc, glm_proc):
            if pr:
                kill(pr)
        for fh in files.values():
            fh.close()
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
