#!/usr/bin/env python3
"""@file Renders a Claude Code session transcript as a readable, prefixed conversation.

A session on disk is a flat list of records -- one JSON object per line in the `.jsonl`
Claude Code writes under ~/.claude/projects/, or the same list as a JSON array (what
`curr.json` holds here). Only `user` and `assistant` records are the conversation; the
rest is the client's bookkeeping (modes, titles, latches, file snapshots) and injected
context (`attachment`). Inside a `user`/`assistant` record, `message.content` is a list
of blocks: `text`, `thinking`, `tool_use`, `tool_result`, or a bare string.

This reads such a file and prints the conversation, every line carrying a four-character
prefix, wrapped to 100 columns. The third character is the direction -- `#` into the agent
from outside it, `>` within the agent's own turn (its words, the tools it calls and what
they return, its thinking) -- and the two before it name the kind:

    ###  a human user's prompt, typed by hand
    >>#  <tag>                a harness wrapper in a user turn: a slash command, a paste
    @@#  injected context     reminders, file reads (one summary line each by default)
    $$>  the agent speaking
    >>>  [tool: Bash]         a tool call the agent made
    <<>  [tool result]        what a tool the agent called returned
    **>  [thinking]           a mark that reasoning happened; its text is never stored

Claude Code keeps only a signature of each thinking block, not the reasoning itself, so
there is nothing to render for it but the mark. By default a tool call shows its input and
its result (each capped by `--truncate`) and each injected-context record is summarized to
one `@@#`/`///` line, the noisy ones dropped; `--no-tools` collapses a tool call and its
result to one marker line, `--no-attachments` drops the context records, and
`--full-attachments` dumps them raw. A `mode` change and a `system` event each render as
one `~~~ … ~~~` divider. It only ever reads its input.

Usage:
    python transcript.py SESSION.jsonl
    python transcript.py curr.json -o curr.txt
    python transcript.py curr.json --keep-tools

@date 08-10-2026-00:10
"""

import argparse
import hashlib
import json
import re
import sys
import textwrap


WIDTH = 100  # the column the longest rendered line may reach, prefix included


# -------------------------------------------------------------------------------------------------
# Loading
# -------------------------------------------------------------------------------------------------

def load_records(path):
    """Answers the records of the file at `path`, be it a JSON array or one object per line.

    A file whose first non-space byte is '[' is read as one JSON array; otherwise each
    non-empty line is parsed on its own, as Claude Code writes the live `.jsonl`.

    @param path  str - the session file
    @return list - the records, in file order
    @date 08-10-2026-00:10
    """
    with open(path, encoding="utf8") as f:
        head = f.read(1)
        f.seek(0)
        if head == "[":
            return json.load(f)
        return [json.loads(line) for line in f if line.strip()]


# -------------------------------------------------------------------------------------------------
# Text helpers
# -------------------------------------------------------------------------------------------------

def as_text(content):
    """Answers the text of a value that is a string, a list of blocks, or neither.

    A tool_result's `content` and an attachment's `rendered` are each a bare string or a
    list of `{type, text}` blocks; anything else is rendered as its JSON.

    @param content  str|list|dict - the value
    @return str
    @date 08-10-2026-00:10
    """
    if isinstance(content, str):
        return content
    if isinstance(content, list):
        return "".join(b.get("text", "") for b in content if isinstance(b, dict))
    return json.dumps(content, ensure_ascii=False)


def clip(text, limit):
    """Answers `text` cut to `limit` characters with a note of what was dropped, or `text`
    whole when `limit` is 0 or it already fits.

    @param text   str - the text
    @param limit  int - the cap, 0 for none
    @return str
    @date 08-10-2026-00:10
    """
    if limit and len(text) > limit:
        return text[:limit] + f"\n[... {len(text) - limit} more chars]"
    return text


def wrap_prose(prefix, text):
    """Answers the lines of `text` word-wrapped to WIDTH and each prefixed with `prefix`.

    A blank line of the source stays a blank line, so paragraph breaks survive. A word is
    broken only when it alone is longer than the line, so ordinary prose keeps its words
    whole while a lone long token -- a path, a table row with no spaces -- still obeys the
    margin.

    @param prefix  str - what every line begins with, e.g. "### "
    @param text    str - the prose
    @return list[str]
    @date 08-10-2026-00:10
    """
    width = max(20, WIDTH - len(prefix))
    out = []
    for raw in text.split("\n"):
        if not raw.strip():
            out.append("")
            continue
        for seg in textwrap.wrap(raw, width, break_long_words=True, break_on_hyphens=False):
            out.append(prefix + seg)
    return out


def prefix_lines(prefix, text):
    """Answers the lines of `text` each prefixed with `prefix`, keeping its own line breaks
    but hard-cutting any line past the 100-column margin into width-sized pieces, so code and
    JSON stay faithful while no line runs long.

    @param prefix  str - what every line begins with
    @param text    str - the body
    @return list[str]
    @date 08-10-2026-02:40
    """
    width = max(20, WIDTH - len(prefix))
    out = []
    for raw in text.split("\n"):
        if len(raw) <= width:
            out.append(prefix + raw)
        else:
            out += [prefix + raw[k:k + width] for k in range(0, len(raw), width)]
    return out


# The wrappers the harness puts around content inside a user turn: a slash command and its
# parts, local-command output, a paste, an injected reminder. Their text is not the user's
# own prose, so it renders under `>>#` rather than `###`. 08-10-2026-01:40
WRAP_TAGS = ["local-command-caveat", "command-name", "command-message", "command-args",
             "local-command-stdout", "command-contents", "pasted_content", "system-reminder"]

# A whole wrapper block: an opening tag of a known name, its body, and a closing tag of the
# same name, each tag allowed trailing attributes (a paste repeats its id on both). 08-10-2026
_WRAP_RE = re.compile(r"<(" + "|".join(WRAP_TAGS) + r")(?:\s[^>]*)?>(.*?)</\1(?:\s[^>]*)?>",
                      re.DOTALL)


def wrap_tag_block(tag, inner):
    """Answers the `>>#` lines of one wrapper block: its tag, its body indented under it, and
    its closing tag; an empty body is one line.

    @param tag    str - the tag name, attributes dropped
    @param inner  str - the text between the tags
    @return list[str]
    @date 08-10-2026-01:40
    """
    inner = inner.strip("\n")
    if not inner.strip():
        return [f">># <{tag}></{tag}>"]
    return [f">># <{tag}>"] + wrap_prose(">>#    ", inner) + [f">># </{tag}>"]


def render_user_text(text):
    """Answers the lines of a user turn, the harness wrappers rendered under `>>#` and the
    user's own prose under `###`, in the order they appear.

    @param text  str - the user record's text
    @return list[str]
    @date 08-10-2026-01:40
    """
    out, pos = [], 0
    for m in _WRAP_RE.finditer(text):
        before = text[pos:m.start()].strip("\n")
        if before.strip():
            out += wrap_prose("### ", before)
        out += wrap_tag_block(m.group(1), m.group(2))
        pos = m.end()
    tail = text[pos:].strip("\n")
    if tail.strip():
        out += wrap_prose("### ", tail)
    return out


# -------------------------------------------------------------------------------------------------
# Block rendering -- the second layer, inside a user/assistant message
# -------------------------------------------------------------------------------------------------

def render_block(block, opt, prose_pfx):
    """Answers the lines a single content block renders to, prose carrying `prose_pfx`.

    `text` and a bare string are prose. `thinking` is always a single mark, since its text
    is not stored. `tool_use` and `tool_result` are a marker line, their body shown under
    it only with keep_tools.

    @param block      dict|str - one content block
    @param opt        argparse.Namespace - the render options
    @param prose_pfx  str - the prefix for this record's prose ("### " or "$$> ")
    @return list[str]
    @date 08-10-2026-00:45
    """
    prose = render_user_text if prose_pfx == "### " else lambda t: wrap_prose(prose_pfx, t)
    if isinstance(block, str):
        return prose(block)
    kind = block.get("type")
    if kind == "text":
        return prose(block.get("text", ""))
    if kind == "thinking":
        return ["**> [thinking]"]
    if kind == "tool_use":
        head = f">>> [tool: {block.get('name', '?')}]"
        if opt.no_tools:
            return [head]
        body = json.dumps(block.get("input", {}), indent=2, ensure_ascii=False)
        return [head] + prefix_lines(">>> ", clip(body, opt.truncate))
    if kind == "tool_result":
        if opt.no_tools:
            return ["<<> [tool result]"]
        return ["<<> [tool result]"] + prefix_lines("<<> ", clip(as_text(block.get("content", "")),
                                                                  opt.truncate))
    return [f"**> [{kind}]"]


# -------------------------------------------------------------------------------------------------
# Record rendering -- the top layer
# -------------------------------------------------------------------------------------------------

def message_lines(rec, opt):
    """Answers the rendered lines of a `user` or `assistant` record, its blocks separated by
    a blank line, or [] when nothing of it survives `opt`.

    @param rec  dict - the record
    @param opt  argparse.Namespace - the render options
    @return list[str]
    @date 08-10-2026-00:10
    """
    content = rec.get("message", {}).get("content")
    blocks = content if isinstance(content, list) else [content]
    prose_pfx = "$$> " if rec["type"] == "assistant" else "### "
    out = []
    for b in blocks:
        lines = render_block(b, opt, prose_pfx)
        if any(ln.strip() for ln in lines):
            if out:
                out.append("")
            out += lines
    return out + [""] if out else []


def system_lines(rec):
    """Answers a one-line `~~~ … ~~~` divider for a `system` record, or [] when it carries
    nothing to show.

    A `compact_boundary` is named with its token drop, since a trimming tool downstream
    looks for it; any other system event shows its text, trimmed to one line.

    @param rec  dict - a record of type 'system'
    @return list[str]
    @date 08-10-2026-01:05
    """
    if rec.get("subtype") == "compact_boundary":
        meta = rec.get("compactMetadata", {})
        pre, post = meta.get("preTokens", "?"), meta.get("postTokens", "?")
        return ["", f"~~~ compacted here ({pre} -> {post} tokens) ~~~", ""]
    content = rec.get("content")
    if isinstance(content, str) and content.strip():
        text = " ".join(content.split())
        return ["", f"~~~ {text[:88]} ~~~", ""]
    return []


# Injected-context subtypes carrying nothing worth a line: a repeating reminder, not an
# event. They are dropped even when attachments are shown. 08-10-2026-01:05
ATTACHMENT_NOISE = {"total_tokens_reminder", "silent_turn_reminder", "auto_mode"}


def attachment_summary(rec):
    """Answers a (kind, summary) pair for an injected-context record, or None to drop it.

    `kind` is a short word naming the block's family -- env, sys, sess, attr, tools,
    skills, model, date, rules -- and keys its `///` id; `summary` is the one line that
    stands for it. The noise kinds are dropped; an unknown kind stands for itself.

    @param rec  dict - a record of type 'attachment'
    @return tuple[str, str]|None
    @date 08-10-2026-02:10
    """
    att = rec.get("attachment", {}) or {}
    st = att.get("type", "attachment")
    if st in ATTACHMENT_NOISE:
        return None
    if st == "environment":
        s = att.get("snapshot", {}) or {}
        shell = (s.get("shell", "") or "").split("(")[0].strip()
        parts = [s.get("workingDirectory"), s.get("platform"), s.get("osVersion"), shell]
        return "env", " · ".join(p for p in parts if p) or "environment"
    if st == "prompt_snapshot":
        sp = att.get("systemPrompt")
        n = len("".join(sp)) if isinstance(sp, list) else len(sp) if isinstance(sp, str) else 0
        return "sys", f"system prompt snapshot (~{n} chars, not shown)"
    if st == "session_context":
        ctx = att.get("context", {}) or {}
        email = re.search(r"[\w.+-]+@[\w-]+(?:\.[\w-]+)+", ctx.get("userEmail", "") or "")
        branch = re.search(r"Current branch:\s*(\S+)", ctx.get("gitStatus", "") or "")
        bits = [b for b in [f"user {email.group(0)}" if email else None,
                            f"git {branch.group(1)}" if branch else None] if b]
        return "sess", " · ".join(bits) or "session context"
    if st == "remote_session_change":
        m = re.search(r"(?:Opus|Sonnet|Haiku|Fable) [\d.]+", str(att.get("commit", "")))
        return "attr", "git attribution footer" + (f" · {m.group(0)}" if m else "")
    if st == "skill_listing":
        return "skills", f"{att.get('skillCount', '?')} skills available"
    if st == "deferred_tools_delta":
        added, loaded = att.get("addedNames"), att.get("surfacedNames")
        bits = [b for b in [f"{len(added)} tools available" if isinstance(added, list) else None,
                            "loaded " + ", ".join(loaded) if isinstance(loaded, list) and loaded
                            else None] if b]
        return "tools", "; ".join(bits) or "tools delta"
    if st == "deferred_tools_record":
        return "tools", "tool inputs recorded"
    if st == "model":
        idn = att.get("identity")
        name = idn.get("marketingName") if isinstance(idn, dict) else idn
        return "model", name if isinstance(name, str) else "model identity"
    if st == "date":
        return "date", str(att.get("date", "?"))
    if st == "instructions":
        f = att.get("files")
        return "rules", f"{len(f)} instruction files" if isinstance(f, list) else "instructions"
    return st, st


def known_id(kind, summary):
    """Answers the `///` id of a known block: its kind, a colon, and a fresh short hash of
    its summary. Identical summaries share an id, so a block that recurs is named once and
    referenced after; the hash is ours, carrying none of the original's opaque tokens.

    @param kind     str - the block's family word
    @param summary  str - the one line that stands for the block
    @return str
    @date 08-10-2026-02:10
    """
    return f"{kind}:{hashlib.blake2b(summary.encode('utf8'), digest_size=4).hexdigest()}"


def header_lines():
    """Answers the legend that opens the render, naming what each line prefix means.

    @return list[str]
    @date 08-10-2026-01:20
    """
    return [
        "~~~ legend: the third prefix char is direction -- # into the agent, > within its "
        "turn ~~~",
        "  ###  a human user's prompt, typed by hand",
        "  >>#  a harness wrapper in a user turn: a slash command, local output, or a paste",
        "  ///  a harness-known block (system prompt, env, tools, …): defined once in the "
        "dictionary,",
        "       referenced by id where it recurs",
        "  @@#  injected context, raw (only with --full-attachments)",
        "  $$>  the agent speaking",
        "  >>>  a tool call the agent made            (>>> [tool: NAME])",
        "  <<>  what a tool the agent called returned (<<> [tool result])",
        "  **>  a mark that the agent thought; its reasoning text is never stored",
        "  ~~~  a divider: a mode change, a system event, or a compaction",
    ]


def render(records, opt):
    """Answers the whole transcript as one string, under the options in `opt`.

    A legend opens it, then a dictionary of the known blocks seen, then the conversation,
    where each known block is a `/// <id>` reference into that dictionary.

    @param records  list - the session records
    @param opt      argparse.Namespace - the render options
    @return str
    @date 08-10-2026-02:10
    """
    body, defs, last_mode = [], {}, None
    for rec in records:
        t = rec.get("type")
        if t in ("user", "assistant"):
            body += message_lines(rec, opt)
        elif t == "system":
            body += system_lines(rec)
        elif t == "mode":
            m = rec.get("mode")
            if m and m != last_mode:
                body += [f"~~~ mode: {m} ~~~", ""]
                last_mode = m
        elif t == "attachment" and not opt.no_attachments:
            if opt.full_attachments:
                body += prefix_lines("@@# ", clip(as_text(rec.get("rendered", "")),
                                                  opt.truncate)) + [""]
            else:
                s = attachment_summary(rec)
                if s:
                    i = known_id(*s)
                    defs.setdefault(i, s[1])
                    body += [f"/// {i}", ""]
    dictionary = []
    if defs:
        dictionary = ["~~~ known blocks (/// ids below resolve here) ~~~"]
        for i, summary in defs.items():
            dictionary += textwrap.wrap(summary, WIDTH, initial_indent=f"/// {i} = ",
                                        subsequent_indent="///       ", break_long_words=True,
                                        break_on_hyphens=False)
        dictionary += [""]
    return "\n".join(header_lines() + [""] + dictionary + ["~~~ conversation ~~~", ""] + body)


# -------------------------------------------------------------------------------------------------
# Entry
# -------------------------------------------------------------------------------------------------

def parse_args(argv):
    """Answers the parsed options for `argv`. @date 08-10-2026-00:10"""
    p = argparse.ArgumentParser(description="Render a Claude Code transcript as a prefixed "
                                            "conversation.")
    p.add_argument("input", help="a session .jsonl, or a curr.json array")
    p.add_argument("-o", "--out", help="write here instead of stdout")
    p.add_argument("--no-tools", action="store_true",
                   help="collapse tool calls and results to a one-line marker")
    p.add_argument("--no-attachments", action="store_true",
                   help="drop injected-context records entirely")
    p.add_argument("--full-attachments", action="store_true",
                   help="dump injected context raw under '@@#' instead of one-line summaries")
    p.add_argument("--all", action="store_true",
                   help="verbose: tool bodies (the default) and attachments dumped raw")
    p.add_argument("--truncate", type=int, default=2000, metavar="N",
                   help="cap each tool body at N chars (default 2000; 0 for no cap)")
    opt = p.parse_args(argv)
    if opt.all:
        opt.no_tools = False
        opt.full_attachments = True
    return opt


def main(argv):
    """Loads the input, renders it and writes the result. @date 08-10-2026-00:10"""
    opt = parse_args(argv)
    text = render(load_records(opt.input), opt)
    if opt.out:
        with open(opt.out, "w", encoding="utf8") as f:
            f.write(text)
        print(f"wrote {opt.out} ({len(text)} chars)", file=sys.stderr)
    else:
        # Windows' console defaults to cp1252, which cannot encode the conversation's
        # arrows and dashes; write the bytes as UTF-8 regardless of the console's codepage.
        # 08-10-2026-00:10
        sys.stdout.buffer.write(text.encode("utf8"))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
