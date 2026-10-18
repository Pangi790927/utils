feature: native compact + own summary (idea, not built)
=====================================================

14-10-2026. An idea for cst/ctxtrim, written down after the measurements below; nothing of it is
implemented yet.

The idea
--------

Trim a session into:

    [native-summary + own-summary, newer half verbatim]

1. Copy the session to be trimmed under a temporary id (the original stays as it is).
2. Run Claude Code's own compaction on the copy, headless:
   `claude -p --model <the session's model> --resume <copy id> "/compact"`, and take the summary it
   writes (the `isCompactSummary` record after the `compact_boundary` record). Delete the copy.
3. Trim the session as cst does now: ctxtrim's own summary of the older half, the newer half
   verbatim.
4. Put both summaries, as two labelled sections, in the one summary record at the top.

Why
---

A trim loses the reasoning (thinking) of the half it keeps, and nothing in the file can bring it
back. The native compact runs on an unmodified copy of the history, so the model writing it has the
whole original context -- full tool outputs (ctxtrim's summarizer only gets a render with every
tool body cut to 500 characters) and the reasoning restored. Its summary is a second account of the
whole session, written from more than ctxtrim's summarizer can see.

It does not save the reasoning itself: a normal summary carries decisions and conclusions, not the
reasoning behind every turn.

What we measured (14-10-2026, a copy of an 85% Opus 5.5 1M session, 2,898 records)
-----------------------------------------------------------------------------------

| Copy, resumed headless with a one-word prompt                  | context tokens |
|----------------------------------------------------------------|----------------|
| live session before copying                                    | 847,274        |
| untrimmed copy                                                 | 855,774        |
| trimmed by cst (newer half verbatim, signatures kept)          | 311,673        |
| the same trimmed copy with every thinking block removed        | 310,164        |

- A resume of an unchanged history restores the reasoning (from the thinking signatures).
- After a trim the kept half's thinking blocks are still sent (checked with a pass-through proxy that
  logged only the request shape: 214 thinking blocks, all with signatures, and
  `context_management: clear_thinking, keep: "all"`), but they add almost nothing: the API restores
  a block's reasoning only when the history before it is the original one. So the loss is on the
  API side; cst keeps the newer half byte for byte (all 1,041 records checked) and cannot do more.
- cst's split itself was fine: by the per-turn usage stored in the file, the kept half held 48% of
  the conversation's growth.
- So a trim lands at roughly "kept visible content": about 0.35 x the start with `--keep 0.5` in a
  session that thinks a lot.

The native compact on the same copy: about 2 min 15 s, a structured 20,764-character summary
(requests, concepts, files, errors, open questions quoted correctly). It deletes nothing: the old
records stay in the file before a `compact_boundary`, the summary points to the full transcript, and
only the last ~20 records are kept verbatim after it.

What does not work
------------------

`/compact <instructions>` asking the model to write down its own reasoning is refused by the API's
safeguards (`[reasoning_extraction]`): the reasoning is deliberately private (Claude Code requests
it with `display: "omitted"`). Do not try to word around it; the compact must be a normal one.

Cost and open points
--------------------

- One more call over the full context per trim, run on the session's own model (another model
  cannot use its signatures); mostly cache reads -- the untrimmed copy, under another id, read 849k
  of its 855k tokens from cache.
- The native summary also covers the newer half kept verbatim: a few thousand duplicated tokens.
- Where it goes: one summary record with two sections ("older half, by ctxtrim" / "whole session, by
  Claude Code's compact"), or the native summary alone in place of ctxtrim's (a simpler variant,
  which also avoids ctxtrim summarizer's failure of answering the transcript instead of
  summarizing it).
- To build it: a `--compact` option in cst; the copy, the headless compact and the extraction in
  ctxtrim; then a test like the one above (resume the result, check the landing and that early
  details are still known).
