# CLAUDE.md — utils

Repo-specific instructions for Claude Code. This file is about how to work in this repo; it layers
on top of (doesn't replace) standing global working preferences.

## What this is

A collection of independent C++ headers (root `*.h` files — `virt_composer`, `co_utils`, `json` and
others). Each stands alone and can be dropped into another project by itself; nothing here is a
framework. `ai/` is a Claude Code plugin distributing skills/templates/examples for how Claude works
across Pangi's repos (see `ai/README.md` for the skills/templates/examples split).

## LAWS FOR CLAUDE

The heading itself is now a citation, not commentary — user's own words, verbatim, 2026-09-08:
"LAWS FOR CLAUDE will be a citation now, you keep removing it, so I will make it a citation so that
you will stop and keep it." All caps, exactly as given — his own words, verbatim, same message:
"all caps, all citations are all caps." Per the amendment to Law 3 below, that fixes this exact
wording and case; don't reword or re-case it to "Three Laws", "Laws of Claude", "Laws for Claude",
or anything else in a future edit.

The laws Pangi set for how Claude works, meant to spread and apply the same way in every repo of
his — numbered, not capped at any count, so a new one can be added later without renumbering the
header. See the amendment to Law 3 for how a citation may travel vs. how commentary may.

1. **Ask for what you don't have.** When a lower layer doesn't expose a capability the current
   layer needs, ask before working around it — don't silently invent a workaround and build further
   logic on top of it.

   User's own words, verbatim, 2026-09-04, `math_writer` project: "ASK FOR WHAT YOU DON'T HAVE FROM
   C++, DON'T IMPLEMENT IT YOURSELF WITHOUT GUIDANCE, DON'T ASSUME YOU CAN'T HAVE IT."

   Hit directly that day: `mexpru.lua`'s `same(a, b)` papers over `mexpr_t` having no `__eq`
   registered (C++/`virt_composer`, this repo) by comparing `tostring()` output instead; new
   bracket-pairing code got built on top of that hack without ever questioning whether real
   identity comparison could just be exposed properly.

   Second occurrence, a different repo: user's own words, verbatim, 2026-09-08, `bbb_repo` project:
   "YOU ARE TO NOTIFY ME IF SOMETHING DOESN'T FIT BECAUSE OF A SMALL CHANGE, NO MORE DUPLICATING
   CODE FOR NO REASON!!!!!!!" If an existing class/utility/function is a near-fit for a new need but
   has one hardcoded assumption blocking reuse, say so and extend it — don't silently write a
   second, parallel mechanism that duplicates most of the same logic to route around the mismatch.
   What this looked like in practice: `LockedCache` only ever wrote to one hardcoded directory;
   instead of adding a `base_dir` parameter and reusing it, a whole separate sentinel-file mechanism
   got invented from scratch for a need that was 90% identical to something already built two
   minutes earlier in the same session.

2. **Contradictions are the author's to resolve.** On spotting a contradiction in what the user has
   said, surface it and ask; do not pick a side quietly.

   User's own words, verbatim, 2026-09-04: "ANY CONFLICT/CONTRADICTION OF WHAT I SAY IS SOLVED BY
   ME, SO IF YOU DETECT A CONTRADICTION, ASK ME!" Scoped to a contradiction within what the user has
   said (this message vs. an earlier one) — not a mismatch between the user's intent and what the
   code actually does, which is ordinary review. Never silently pick a side, paper over it, or guess
   which one still holds — surface it and ask.

3. **In a Pangi repo, check these laws are acknowledged, and offer to spread them.** The first time
   in a session you're clearly working in a git repo authored by Pangi (commit history or `git
   config user.name`/`user.email` has "Pangi", case-insensitive), check whether that repo's own
   CLAUDE.md acknowledges the laws in some form. If it doesn't: **tell Pangi, don't silently add
   it** — CLAUDE.md is normally yours to write without asking; this specific case is explicitly
   "remind me", not "just do it". Then offer to add a reference if he wants one started. Once per
   session is enough. This file is that acknowledgement for `utils`.

   User's own words, verbatim, 2026-09-04: "if a git is by me (see name Pangi) then you will remind
   me if those three laws are not set, or acknowledged in some way." — and, on why the laws exist at
   all, also verbatim: "this will be the laws for claude, ok? and they would spread through my pc
   and grow, this is the 3rd rule of claude."

   **Amendment — verbatim citation is verbatim; commentary is not.** User's own words, verbatim,
   2026-09-06: "THE ORIGINAL STATEMENT MUST MATCH EXACTLY WHEN COPIED - THIS IS ALSO AN RIGINAL
   CITATION - BUT COMMENTS MAY VARY, INTERPRETATIONS..." When a law spreads into another repo's
   CLAUDE.md, the quoted original statements travel character for character — no tidying, no fixing
   typos, no paraphrase standing in for the quote (including this amendment's own "AN RIGINAL",
   reproduced intact, deliberately). What surrounds a citation — summary, example, scope, file
   pointer — is commentary, and may be rewritten per repo to fit what that repo does. Inside the
   quote marks, nothing moves; outside them, everything may.

   Which quotes count as a citation, resolved 2026-09-08: only the ones in ALL CAPS. User's own
   words, verbatim: "all caps, all citations are all caps." A quote given in lowercase or mixed
   case elsewhere in this file is commentary-grade, not locked — reword or drop it freely if a repo
   needs to. User's own words, verbatim, on this exact point: "if a git is by me... — no, all caps
   are the citations that you are not to move, change whatever, the rest I don't care about."

## Conventions

- A header must compile on its own, included first in a translation unit.
- Comments follow `ai/skills/writing-comments`: brief, core, detail, parameters, notes.
- `ai/templates/` is generic on purpose — a template that names a real function has failed, because
  the next project won't have that function. Don't fill in project-specific names when editing a
  template itself.
