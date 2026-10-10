#!/usr/bin/env python3
"""@file Trims a Claude Code session: summarizes the older half, keeps the recent half.

transcript.py reads a session; this rewrites one. It splits the conversation near the middle
(by characters, snapped to a human-prompt boundary), summarizes only the OLDER half, keeps the
RECENT half verbatim, and writes a NEW session file -- never touching the original -- that
`claude --resume` can continue. Driven at ~60% context, it lands the session near ~50%, since
the summary of the older half is itself ~10%. A later run repeats on the new older half, so a
conversation never has to end for lack of room.

How it decides and summarizes:

  - The split falls only at a human prompt -- a user turn the person typed, not a tool result --
    so a tool call and its result are never split.
  - The WHOLE conversation is rendered to text (through transcript.py) with a MARKER at the
    split, and handed to `claude -p`: it is told to summarize only ABOVE the marker, using the
    verbatim part below only as context. So the summary of the older half is informed by what it
    led to. It comes back as a compact structured block (intent, concepts, files, decisions,
    errors, asks, open items), modelled on the host's own `/compact`.
  - The older half's records are replaced by that one `isCompactSummary` user message; the recent
    half is copied with its `message` untouched -- an assistant turn's thinking signature must
    survive for the resume to validate -- its first turn re-pointed onto the summary.

Usage:
    python ctxtrim.py SESSION.jsonl --dry-run     # show the older-half summary; write nothing
    python ctxtrim.py SESSION.jsonl --keep 0.5    # keep the recent 50%, summarize the older 50%
    # then: claude --resume <printed id>

@date 08-10-2026-13:30
"""

import argparse
import datetime
import json
import os
import subprocess
import sys
import uuid
from types import SimpleNamespace

import transcript


# -------------------------------------------------------------------------------------------------
# The chain: where a cut may fall
# -------------------------------------------------------------------------------------------------

def human_prompt_indices(records):
    """Answers the indices of the records that are human prompts -- a user turn the person
    typed -- which are the only places a cut is safe, a tool result never among them.

    @param records  list - the session records, in file order
    @return list[int]
    @date 08-10-2026-03:10
    """
    out = []
    for i, r in enumerate(records):
        if r.get("type") != "user":
            continue
        c = r.get("message", {}).get("content")
        if isinstance(c, str):
            out.append(i)
        elif isinstance(c, list) and not any(isinstance(b, dict) and b.get("type") == "tool_result"
                                             for b in c):
            out.append(i)
    return out


def last_message_uuid(records):
    """Answers the uuid of the last user/assistant record in `records`, or None.

    @param records  list - a run of records
    @return str|None
    @date 08-10-2026-03:10
    """
    for r in reversed(records):
        if r.get("type") in ("user", "assistant") and r.get("uuid"):
            return r["uuid"]
    return None


# -------------------------------------------------------------------------------------------------
# The summarizer: one structured pass through headless `claude -p`
# -------------------------------------------------------------------------------------------------

def ask_claude(prompt, text, model, cli="claude"):
    """Answers what a headless agent replies to `prompt` with `text` on its stdin. `cli` picks the
    agent: "claude" runs `claude -p` on `model`; "glm" runs `claude-glm.cmd -p` on its own model,
    so a session can be summarized by the same bot that wrote it.

    @param prompt  str - the instruction
    @param text    str - the material, piped in
    @param model   str - the model id (used by "claude" only)
    @param cli     str - "claude" or "glm"
    @return str
    @throws RuntimeError when the call fails
    @date 08-10-2026-16:00
    """
    if cli == "glm":
        # cmd.exe re-parses its command line, so newlines and < > in the prompt would be mangled;
        # the instructions travel on stdin ahead of the material instead. 08-10-2026-16:00
        cmd = ["cmd", "/c", "claude-glm.cmd", "-p",
               "Follow the INSTRUCTIONS at the top of the input exactly."]
        text = "INSTRUCTIONS:\n" + prompt + "\n\nINPUT:\n" + text
    else:
        cmd = ["claude", "-p", "--model", model, prompt]
    r = subprocess.run(cmd, input=text, capture_output=True, text=True, encoding="utf8",
                       errors="replace", timeout=900)
    if r.returncode != 0:
        raise RuntimeError(f"claude -p failed ({r.returncode}): {(r.stderr or '')[:500]}")
    return r.stdout.strip()


MARKER = "\n<<<<<< KEEP EVERYTHING BELOW THIS LINE VERBATIM -- DO NOT SUMMARIZE IT >>>>>>\n"

HALF_PROMPT = (
    "Below is a software-development conversation. Everything BELOW the line that reads "
    "'<<<<<< KEEP EVERYTHING BELOW THIS LINE VERBATIM' stays in the continued session exactly as "
    "is -- do NOT summarize it; use it only to understand what the earlier part led to. Summarize "
    "everything ABOVE that line into a compact, structured summary (like the host's /compact), "
    "using these sections and dropping any that would be empty:\n"
    "1. Primary requests and intent.\n"
    "2. Key technical concepts.\n"
    "3. Files and code touched -- each path and what changed.\n"
    "4. Decisions made and why.\n"
    "5. Errors and fixes.\n"
    "6. User asks.\n"
    "7. Open or pending items.\n"
    "Keep names, paths and numbers exact, and anything the verbatim part below depends on. Output "
    "only the summary."
)

# Opens the summary record, so the resumed model reads it as background with the recent half
# following verbatim. 08-10-2026-13:30
SUMMARY_PREFIX = ("[Summary of the earlier half of this conversation; the recent half follows "
                  "verbatim below.]\n\n")


def summarize_older(text, model, cli="claude"):
    """Answers a compact structured summary of the older half -- the part above the marker in
    `text` -- the recent half below the marker left for the model to read verbatim.

    @param text   str - the whole conversation rendered, with MARKER at the split
    @param model  str - the model id
    @param cli    str - which agent summarizes ("claude" or "glm")
    @return str
    @date 08-10-2026-16:00
    """
    return ask_claude(HALF_PROMPT, text, model, cli)


def content_len(rec):
    """Answers a character-count proxy for a record's size. @date 08-10-2026-13:30"""
    m = rec.get("message", {})
    c = m.get("content") if isinstance(m, dict) else None
    if isinstance(c, str):
        return len(c)
    return len(json.dumps(c)) if c is not None else 0


def split_index(records, keep_fraction):
    """Answers the record index that splits the conversation so the recent `keep_fraction` (by
    characters) is kept verbatim and the older part is summarized, snapped to the first human-prompt
    boundary at or after that point. @date 08-10-2026-13:30"""
    total = sum(content_len(r) for r in records if r.get("type") in ("user", "assistant"))
    older_target = total * (1.0 - keep_fraction)
    hp = set(human_prompt_indices(records))
    acc = 0
    for i, r in enumerate(records):
        if r.get("type") in ("user", "assistant"):
            acc += content_len(r)
        if i in hp and acc >= older_target:
            return i
    return max(human_prompt_indices(records) or [len(records)])


# -------------------------------------------------------------------------------------------------
# Building the new session
# -------------------------------------------------------------------------------------------------

def resession(rec, sid):
    """Answers a shallow copy of `rec` carrying the new session id, its `message` and every
    other field untouched.

    @param rec  dict - a record to keep
    @param sid  str - the new session id
    @return dict
    @date 08-10-2026-03:10
    """
    out = dict(rec)
    out["sessionId"] = sid
    if "session_id" in out:
        out["session_id"] = sid
    return out


def summary_record(text, uuid_, parent, sid, proto):
    """Answers an `isCompactSummary` user record carrying `text`, linked under `parent`, with
    the session fields taken from `proto` (an existing record).

    @param text    str - the summary body
    @param uuid_   str - this record's uuid
    @param parent  str - the uuid this record links under
    @param sid     str - the new session id
    @param proto   dict - a record to copy cwd/gitBranch/version from
    @return dict
    @date 08-10-2026-03:10
    """
    now = datetime.datetime.now(datetime.timezone.utc).isoformat().replace("+00:00", "Z")
    return dict(parentUuid=parent, type="user", uuid=uuid_, isCompactSummary=True,
                isSidechain=False, userType="external", sessionId=sid, timestamp=now,
                cwd=proto.get("cwd", ""), gitBranch=proto.get("gitBranch", ""),
                version=proto.get("version", ""),
                message={"role": "user", "content": text})


def build_session(records, split, summary, sid=None):
    """Answers (sid, new_records): the older half (records before `split`) replaced by the one
    `summary` record, and the recent half (records from `split`) kept verbatim, re-chained under
    `sid` (fresh when None, or a fixed fixture id).

    @param records  list - the original session records
    @param split    int - the index where the recent (kept) half begins
    @param summary  str - the structured summary of the older half
    @param sid      str|None - the session id to write under
    @return tuple[str, list]
    @date 08-10-2026-13:30
    """
    sid = sid or str(uuid.uuid4())
    proto = records[0]
    recent = [resession(r, sid) for r in records[split:]]
    s_uuid = str(uuid.uuid4())
    s_rec = summary_record(SUMMARY_PREFIX + summary, s_uuid, None, sid, proto)
    for r in recent:                     # re-point the first kept turn onto the summary
        if r.get("type") in ("user", "assistant"):
            r["parentUuid"] = s_uuid
            break
    return sid, [s_rec] + recent


# -------------------------------------------------------------------------------------------------
# Entry
# -------------------------------------------------------------------------------------------------

def render_with_marker(records, split, truncate):
    """Answers the whole conversation rendered to text with a marker at `split`: the older half,
    then MARKER, then the recent half (its legend header stripped, so there is one clean marker).
    The summarizer sees everything but is told to summarize only above the marker.

    @param records   list - the session records
    @param split     int - the index where the recent half begins
    @param truncate  int - the per-tool-body cap passed to the renderer
    @return str
    @date 08-10-2026-13:30
    """
    opt = SimpleNamespace(no_tools=False, no_attachments=False, full_attachments=False,
                          truncate=truncate)
    older = transcript.render(records[:split], opt)
    recent = transcript.render(records[split:], opt)
    key = "~~~ conversation ~~~"
    if key in recent:
        recent = recent.split(key, 1)[1]
    return older + MARKER + recent


def projects_dir():
    """Answers the Claude Code projects directory for the current working directory's project.
    @return str @date 08-10-2026-03:10"""
    slug = os.getcwd().replace("\\", "-").replace("/", "-").replace(":", "")
    return os.path.join(os.path.expanduser("~"), ".claude", "projects", "C--" + slug.split("C-", 1)[-1])


def parse_args(argv):
    """Answers the parsed options for `argv`. @date 08-10-2026-03:10"""
    p = argparse.ArgumentParser(description="Trim a Claude Code session and write a resumable "
                                            "shorter copy.")
    p.add_argument("input", help="a session .jsonl to trim")
    p.add_argument("--keep", type=float, default=0.5,
                   help="fraction of the conversation (by chars) to keep verbatim as the recent "
                        "half; the older part is summarized")
    p.add_argument("--model", default="claude-opus-5-5", help="model for the summaries (cli claude)")
    p.add_argument("--cli", choices=("claude", "glm"), default="claude",
                   help="which agent writes the summary: claude, or glm via claude-glm.cmd")
    p.add_argument("--truncate", type=int, default=500, help="per-tool-body cap in the middle")
    p.add_argument("--out-dir", help="where to write the new session (default: beside the input)")
    p.add_argument("--sid", help="write under this fixed session id (overwrites that fixture "
                                 "file), instead of a fresh uuid")
    p.add_argument("--dry-run", action="store_true", help="print the summary and write nothing")
    return p.parse_args(argv)


def main(argv):
    """Trims the session named on the command line: summarize the older half, keep the recent half.
    @date 09-10-2026"""
    opt = parse_args(argv)
    records = transcript.load_records(opt.input)
    split = split_index(records, opt.keep)
    text = render_with_marker(records, split, opt.truncate)
    print(f"{len(records)} records; split at index {split} (keep recent {opt.keep:.0%}); "
          f"summarizer input {len(text)} chars", file=sys.stderr)

    summary = summarize_older(text, opt.model, opt.cli)
    sys.stdout.buffer.write(f"=== summary (older half) ===\n{summary}\n".encode("utf8"))

    if opt.dry_run:
        print("\n[dry-run: no file written]", file=sys.stderr)
        return 0

    sid, new_records = build_session(records, split, summary, opt.sid)
    out_dir = opt.out_dir or os.path.dirname(os.path.abspath(opt.input))
    path = os.path.join(out_dir, sid + ".jsonl")
    with open(path, "w", encoding="utf8") as f:
        for r in new_records:
            f.write(json.dumps(r, ensure_ascii=False) + "\n")
    print(f"\nwrote {path}\n  {len(new_records)} records; resume with: claude --resume {sid}",
          file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
