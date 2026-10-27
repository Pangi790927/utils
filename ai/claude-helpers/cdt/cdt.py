#!/usr/bin/env python3
"""@file cdt -- Claude drop thought: drops the reasoning from a Claude Code session, in place, and
keeps everything else in it word for word.

On Opus 5.5 a session's earlier thinking stays in its context, and at high effort it is most of
it: 60% of swarm's Reviewer at max (15-10-2026). cdt takes every thinking block out of what
`claude --resume` loads, and nothing else -- each prompt, answer, tool call and result stays as it
was. No model is asked anything, so it is instant and free; the next turn writes the smaller
context to the prompt cache once.

  - The API allows it: thinking may be removed oldest first, from the end, or all of it; what it
    refuses is a gap (the preserved-thinking docs). cdt removes all of it, so the API's prefix
    check has nothing left to reject. Thinking made after cdt is kept as usual, until the next.
  - Claude Code writes each block of an answer as a record of its own, and puts the records of one
    answer back together by its message id, so a record only cut out of the chain could come back.
    A record holding only thinking is therefore deleted, and what hung on it is re-pointed onto
    its parent; a record holding thinking beside other blocks keeps the others.
  - The file as it was is kept beside it as `<session>.jsonl.cdt-bak`. Putting it back is only
    safe before the next turn: thinking restored after that invalidates what was made without it.
  - Only what resume loads is touched: a log before the chain's root, or a branch the conversation
    went back from, stays byte for byte.

--dry only reports: how much thinking there is, how much of it the API still counts -- a block
made before the history ahead of it was edited, by a ctxtrim summary say, is dropped by the API
already -- and where the next turn would start.

Usage:
    cdt [--dry]                    inside a session (`! cdt`): through cst's switch, a helper
                                   waits until it is idle, ends it, drops, and brings it back by
                                   itself -- on Linux, through the cst.bash function as cst does
    cdt SESSION.jsonl|ID [--dry]   a session no claude is writing to

@date 15-10-2026-12:00, 24-10-2026-19:25
"""

import argparse
import datetime
import json
import os
import sys
from types import SimpleNamespace

HERE = os.path.dirname(os.path.abspath(__file__))
HELPERS = os.path.dirname(HERE)
sys.path.insert(0, HELPERS)
sys.path.insert(0, os.path.join(HELPERS, "cst"))
import ctxtrim      # noqa: E402
import cst          # noqa: E402

LOG = os.path.join(HERE, "cdt.log")
cst.LOG = LOG       # the cst helpers cdt reuses log to cdt.log as well. 15-10-2026-12:00

THINKING = ("thinking", "redacted_thinking")

# Every signature is about SIG_FLOOR characters however little it carries, and past that about
# SIG_PER_TOKEN characters a token of reasoning: measured on Opus 5.5 by the context a drop freed,
# 31 blocks of a max-effort reviewer within 2% and 3 tiny ones within a few dozen tokens. Only the
# estimates use them. 15-10-2026-12:45
SIG_FLOOR = 900
SIG_PER_TOKEN = 4.0


# -------------------------------------------------------------------------------------------------
# What there is to drop
# -------------------------------------------------------------------------------------------------

def thinking_of(rec):
    """Answers the thinking blocks of a record, plain or redacted; [] when it has none.
    @date 15-10-2026-12:00"""
    return [b for kind in THINKING for b in ctxtrim.content_blocks(rec, kind)]


def block_tokens(block):
    """Answers about how many tokens of context a thinking block holds: its text, which Claude
    Code stores empty, and its signature past the floor every signature has, or a redacted block's
    data. @date 15-10-2026-12:45"""
    sig = len(block.get("signature") or block.get("data") or "")
    return (len(block.get("thinking") or "") + max(0, sig - SIG_FLOOR)) / SIG_PER_TOKEN


def is_edit(rec):
    """Answers whether a record marks the history before what follows it as edited: a trim's or
    a compaction's summary, or a compaction's boundary. @date 15-10-2026-12:45"""
    return bool(rec.get("isCompactSummary")) or rec.get("subtype") == "compact_boundary"


def when(rec):
    """Answers a record's timestamp as a datetime, None when it has none. @date 15-10-2026-12:00"""
    t = rec.get("timestamp")
    return datetime.datetime.fromisoformat(t.replace("Z", "+00:00")) if t else None


def context_of(rec):
    """Answers the tokens an assistant record's request ended at -- fresh, cached and written to
    the cache, plus its answer -- or 0 when it carries no usage. @date 15-10-2026-12:00"""
    u = rec.get("message", {}).get("usage") if rec.get("type") == "assistant" else None
    if not isinstance(u, dict):
        return 0
    return sum(u.get(k, 0) for k in ("input_tokens", "cache_creation_input_tokens",
                                     "cache_read_input_tokens", "output_tokens"))


def survey(records):
    """@brief Answers the reasoning in what resume loads of `records`.

    A block counts as stale when a summary or a compaction boundary ahead of it in what resume
    loads was written after it: the history before it was edited, so the API's prefix check fails
    it and the API drops it already.

    @param records  list - the session records, in file order
    @return SimpleNamespace - `loaded` the uuids resume loads; `whole` the loaded records holding
            only thinking, `mixed` those holding it beside other blocks; `blocks` the thinking
            blocks, `tokens` about how much context they hold and `stale` how much of that the
            API drops already; `context` the tokens the last turn ended at, 0 when unknown;
            `root` the uuid of the chain's root
    @date 15-10-2026-12:45
    """
    s = SimpleNamespace(loaded=ctxtrim.loaded_uuids(records), whole=set(), mixed=set(), blocks=0,
                        tokens=0.0, stale=0.0, context=0, root=None)
    root = ctxtrim.chain_root(records)
    s.root = records[root].get("uuid") if root is not None else None
    edited = None
    for r in records:
        if r.get("uuid") not in s.loaded:
            continue
        t, blocks = when(r), thinking_of(r)
        if is_edit(r) and t and (edited is None or t > edited):
            edited = t
        if blocks:
            size = sum(block_tokens(b) for b in blocks)
            s.blocks, s.tokens = s.blocks + len(blocks), s.tokens + size
            if edited and t and t < edited:
                s.stale += size
            whole = len(blocks) == len(r["message"]["content"])
            (s.whole if whole else s.mixed).add(r["uuid"])
        s.context = context_of(r) or s.context
    return s


def report(s, dry):
    """Answers what dropping does, or would do, to the session `s` surveys, in a few lines.
    @date 15-10-2026-12:45"""
    if not s.blocks:
        return "cdt: nothing to drop -- there is no thinking in what resume loads"
    out = [f"cdt: {'would drop' if dry else 'dropped'} {s.blocks} thinking blocks "
           f"({len(s.whole)} records deleted, {len(s.mixed)} trimmed), "
           f"~{s.tokens:,.0f} tokens of reasoning"]
    if s.stale:
        out.append(f"     ~{s.stale:,.0f} of them the API drops already: the history before "
                   f"them was edited")
    if s.context:
        after = max(0.0, s.context - (s.tokens - s.stale))
        out.append(f"     the last turn ended at {s.context:,} tokens; the next starts at about "
                   f"{after:,.0f}, {after / s.context:.0%} of that")
    return "\n".join(out)


# -------------------------------------------------------------------------------------------------
# The drop
# -------------------------------------------------------------------------------------------------

def without_thinking(lines, records, s):
    """@brief Answers the session's lines with the reasoning resume loads dropped: each record in
    `s.whole` deleted, whatever named it as parent or leaf re-pointed onto its nearest kept
    ancestor, and each record in `s.mixed` rewritten without its thinking blocks. Every other line
    stays byte for byte.

    @param lines    list[bytes] - the session file's lines
    @param records  list[dict] - the records they hold
    @param s        SimpleNamespace - their survey
    @return list[bytes]
    @date 15-10-2026-12:00
    """
    parent = {r["uuid"]: r.get("parentUuid") for r in records if r.get("uuid")}

    def kept(u):
        while u in s.whole:
            u = parent.get(u)
        return u

    out, it = [], iter(records)
    for line in lines:
        if not line.strip():
            out.append(line)
            continue
        r = next(it)
        if r.get("uuid") in s.whole:
            continue
        new = dict(r)
        for key in ("parentUuid", "leafUuid"):
            if new.get(key) in s.whole:
                new[key] = kept(new[key])
        if r.get("uuid") in s.mixed:
            new["message"] = dict(r["message"], content=[
                b for b in r["message"]["content"]
                if not (isinstance(b, dict) and b.get("type") in THINKING)])
        out.append(line if new == r else ctxtrim.record_line(new))
    return out


def unsound(lines, s):
    """Answers why the dropped session `lines` would not resume as the original minus its
    reasoning -- resume reaching another root, or a different set of records, or a record still
    naming a deleted one -- or None when it would. @date 15-10-2026-12:00"""
    records = [json.loads(line) for line in lines if line.strip()]
    root = ctxtrim.chain_root(records)
    if root is None or records[root].get("uuid") != s.root:
        return "resume would no longer reach the session's root from the last message"
    if ctxtrim.loaded_uuids(records) != s.loaded - s.whole:
        return "resume would load other records than before, less the thinking"
    for r in records:
        for key, value in r.items():
            if key != "uuid" and isinstance(value, str) and value in s.whole:
                return f"a {r.get('type')} record still names a deleted one in `{key}`"
    return None


def drop_in_place(path):
    """@brief Drops the reasoning resume loads from the session file at `path`, in place, and keeps
    the file as it was beside it, in `path`.cdt-bak.

    The file is left as it is when there is nothing to drop, when the result would not resume as
    the original minus its reasoning, or when the file changed while cdt worked on it.

    @param path  str - a session .jsonl no claude is writing to
    @return tuple[SimpleNamespace, str|None] - its survey, and why it was left as it is (None once
            it is rewritten)
    @date 15-10-2026-12:00
    """
    lines, records = ctxtrim.load_session(path)
    s = survey(records)
    if not s.blocks:
        return s, "there is no thinking in what resume loads"
    out = without_thinking(lines, records, s)
    why = unsound(out, s)
    if why:
        return s, why
    with open(path, "rb") as f:
        if f.read() != b"".join(lines):
            return s, "the session changed while cdt worked on it"
    ctxtrim.write_lines(path + ".cdt-bak", lines)
    ctxtrim.write_lines(path, out)
    return s, None


# -------------------------------------------------------------------------------------------------
# Entry: a named session, or the one cdt runs in
# -------------------------------------------------------------------------------------------------

def on_file(path, dry):
    """Drops the reasoning from the session file at `path`, or with `dry` only reports; answers
    the exit code, 0 when there was reasoning to drop. @date 15-10-2026-12:00"""
    if dry:
        s = survey(ctxtrim.load_session(path)[1])
        print(report(s, True))
        return 0 if s.blocks else 1
    s, why = drop_in_place(path)
    if why:
        print(report(s, False) if not s.blocks else f"cdt: not written: {why}")
        return 1
    print(report(s, False))
    print(f"     the file as it was: {path}.cdt-bak")
    return 0


def helper(o):
    """Switches the session cdt was run from with the drop: once it is idle it ends, its reasoning
    is dropped, and it comes back -- cst's switch, with no summary to make first.
    @date 15-10-2026-12:00, 24-10-2026-19:25"""
    cst.log(f"=== cdt helper: session {o.sid}, claude pid {o.claude_pid}, {cst.shown(o)}")

    def drop(sf):
        s, why = drop_in_place(sf)
        return f"{why} -- nothing changed; it comes back as it was" if why else report(s, False)
    cst.switch(o, drop)


def in_session(dry):
    """Reports on the session cdt runs in, and unless `dry` starts the helper that drops its
    reasoning and brings it back; answers the exit code. @date 15-10-2026-12:00, 24-10-2026-19:25"""
    here = cst.this_session("cdt")
    if not here:
        return 1
    sid, cpid, path = here
    s = survey(ctxtrim.load_session(path)[1])
    print(report(s, True))
    if dry or not s.blocks:
        return 0 if s.blocks else 1
    why = cst.launch_helper(__file__, sid, cpid, [])
    if why:
        print(f"cdt: {why}")
        return 1
    print(f"cdt: it will end, drop and come back by itself shortly. Nothing to do meanwhile; do "
          f"not type in this session until it is back. Log: {LOG}")
    return 0


def main(argv):
    """Drops the reasoning from the session named on the command line, or from the one cdt runs
    in. @date 15-10-2026-12:00, 24-10-2026-19:25"""
    p = argparse.ArgumentParser(description="Drop the reasoning from a Claude Code session, in "
                                            "place, keeping everything else word for word.")
    p.add_argument("session", nargs="?",
                   help="a session .jsonl or its id, one no claude is writing to; left out, the "
                        "session cdt runs in")
    p.add_argument("--dry", action="store_true", help="only report what it would drop")
    cst.helper_options(p)
    o = p.parse_args(argv)
    if o.helper:
        try:
            helper(o)
        except Exception as e:                                          # noqa: BLE001
            cst.log(f"cdt helper failed: {e!r} -- the session file is untouched unless logged "
                    f"above")
        return 0
    if not o.session:
        return in_session(o.dry)
    path = o.session if os.path.isfile(o.session) else cst.find_session(o.session)
    if not path:
        print(f"cdt: no session file for {o.session}")
        return 1
    return on_file(path, o.dry)


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
