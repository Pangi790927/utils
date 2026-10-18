claude-helpers
==============

Tools for looking at, trimming, and driving Claude Code conversations. The through-line is one
problem: **a conversation has a context limit, and we want to work past it** -- to read a long
session cheaply, to trim it so it can continue, and to run long-lived agents (including two
different models working together) that outlive any single context window.

This file is the concept store. It is written to survive the very thing it is about: when a session
compacts and the details fall out of context, the ideas live on here. Keep it current; put lasting
decisions and ideas here, not only in chat.

09-10-2026


Why
---

A Claude Code session is a growing list of turns. Past a point the host compacts it: it summarizes
the whole thing into one small block and keeps only the last few messages, which loses the recent
working detail an agent needs most. The user's idea is finer: when the context is well past half,
split the conversation in the middle, summarize only the older half -- written by a model that sees
the whole conversation, so the summary knows what the older part led to -- keep the recent half
verbatim, and resume. Done repeatedly, a conversation never has to end for lack of room, and an
agent keeps its working memory across every shrink.


The tools
---------

| File                          | What it does                                                     |
|-------------------------------|------------------------------------------------------------------|
| `transcript.py`               | Renders a session `.jsonl` (or a `curr.json` array) as a         |
|                               | readable, 100-column conversation. Reads only; never edits its   |
|                               | input.                                                           |
| `ctxtrim.py`                  | Trims a session the half-keep way, in place: `claude --resume`   |
|                               | then loads a summary of the older half and the recent half       |
|                               | verbatim; the older half's records stay in the file as a log.    |
| `gamedev_loop.py`             | The orchestrator of the two-agent game studio (below): owns the  |
|                               | agent windows, relays turns, trims, recovers, commits.           |
| `game.md`                     | The seed design for the studio's game; copied into the game      |
|                               | directory only when none exists there (the agents own theirs).   |
| `curr.json`, `curr.txt`       | A frozen session and its render: the working test fixture.       |
| `builtin_compact_example.txt` | A real host `/compact` summary, kept for comparison.             |
| `trim_summaries.txt`          | Sample trim output, kept for comparison.                         |
| `cst/`                        | Claude self-trim: `! cst` trims the session it is run from, in   |
|                               | place, and resumes it by itself (below).                         |
| `cdt/`                        | Claude drop thought: drops the reasoning from a session, in      |
|                               | place, every other word kept; `! cdt` does it to the session it  |
|                               | is run from and resumes it (below).                              |
| `paired/`                     | A copy of the game studio's runner, without the game, with a     |
|                               | README on how to reinstate it.                                   |


### cst -- trim the session you are in

Inside a session, `! cst [--model M] [--keep 0.5]`. It reads the session's exact id and process
from `CLAUDE_CODE_SESSION_ID` / `CLAUDE_PID` (it refuses to run without them and never guesses by
file times), starts a detached helper, and returns. The helper summarizes the older half while
the session stays open, waits until it is idle, types `/exit`, trims the file in place once Claude
has closed it (ctxtrim's trim, under the same id), and types the original launch command with
`--resume <id>` into the same terminal. Nothing is thrown away: the older half stays in the file
as a log that resume no longer loads, so there is no `--backup` any more (15-10-2026). Tested on a
throwaway session (10-10-2026, before the trim kept the log): back in about 20 seconds with a
Haiku summary, every early fact still known. Do not type in that terminal while it switches. Log:
`cst/cst.log`. Launchers `cst` / `cst.cmd` also live on the PATH in `~/workspace/claude-utils/`.

### cdt -- drop the reasoning

`cdt SESSION.jsonl|ID` drops every thinking block from what `claude --resume` loads, in place, and
nothing else; inside a session, `! cdt` does it to that session through cst's switch (idle,
`/exit`, drop, resume). `--dry` only reports: how many blocks, about how many tokens of context
they hold, how much of that the API drops already, and where the next turn would start. No model
is asked anything, so it is instant and free. On Opus 5.5 earlier thinking stays in the context,
and at high effort it is most of it: swarm's max-effort Reviewer stood at 681K tokens, ~375K of
them reasoning the API still counted, and `cdt --dry` put its next turn at ~307K (15-10-2026).

A record holding only thinking is deleted, and what hung on it -- or named it as `leafUuid` -- is
re-pointed onto its parent: Claude Code joins the records of one answer by message id, so a record
merely cut out of the chain could come back. A record with thinking beside other blocks keeps the
others. The file as it was is kept as `<session>.jsonl.cdt-bak`; putting it back is only safe
before the next turn. The rewrite is checked before it is written: resume must reach the same root
and load the same records, less the thinking. The estimate counts a signature's ~900-character
floor out, then ~4 characters a token. Tested 15-10-2026 on hand-made sessions and on a real Opus
5.5 one: the resume after the drop worked, its context fell by the thinking, and the earlier
answers were recalled. The in-session switch is cst's own; it was checked with its typing faked,
not live. Log: `cdt/cdt.log`. Launchers `cdt` / `cdt.cmd` also live in `~/workspace/claude-utils/`.

**On Linux** (14-10-2026) Claude Code runs a session under its daemon (`claude daemon run`), on a
terminal of its own owned by a relay process, `claude bg-pty-host --bg-pty-host <socket>` (the
parent of `CLAUDE_PID`). Your terminal only runs a thin client, and the relay's one client is the
daemon, so nothing links a session to the terminal that shows it. So:

- `/exit` is typed through the relay socket, as a client does: frames of
  `[uint32 BE length][uint8 kind][payload]`, kind 0 = raw terminal bytes (typed into the session),
  kind 1 = JSON control (`hello` with the Claude Code version, the screen so far, then `live`;
  `ping`/`pong`, `auth`/`auth-required`, `resize`, `kill`, `exit`). No token is set up on this
  machine, so typing is accepted. Undocumented (seen in 2.1.295): if the relay refuses
  (`auth-required`), is missing, or the session doesn't exit, cst falls back to SIGTERM.
- The session comes back through `cst/cst.bash`, sourced from `~/.bashrc`: it wraps `claude`, and
  when that exits it runs `cst --take-request $PWD`. cst leaves a request in `cst/requests/`
  ("pending" before `/exit`, "ready" after the rewrite) with the session id and the directory it
  was started in; a request for the same directory, at most 15 s old, is consumed and `claude` is
  started again with the same arguments and `--resume <id>` (an earlier resume or continue
  dropped). Matching on directory and time is a heuristic: exiting another `claude` in the same
  directory within those 15 s would take the request. Without the wrapper: `claude --resume <id>`.
- `~/.local/bin/cst` is a link to `cst/cst`, which picks `python3` on Linux and `python` on
  Windows.
- Not supported: sessions hosted by Claude Code's background daemon (opened from the agents view):
  their relay refuses typing, and a session ended from outside is renamed `.orphaned` and can't be
  resumed by id. Run cst from a session started with plain `claude` in a terminal.

A trim loses the reasoning of the half it keeps (an API-side limit, measured 14-10-2026), so it
lands at about 0.35 x the start with `--keep 0.5` in a session that thinks a lot; the idea of adding
Claude Code's native compact summary is in `cst/feature-compact.md`.

### transcript.py line prefixes

The third prefix character is direction -- `#` into the agent from outside, `>` within the agent's
own turn. The two before it name the kind.

| Prefix | Meaning                                                                            |
|--------|------------------------------------------------------------------------------------|
| `###`  | a human user's prompt, typed by hand                                               |
| `>>#`  | a harness wrapper inside a user turn: a slash command, local output, or a paste    |
| `///`  | a harness-known block (system prompt, env, tools): defined once in a top           |
|        | dictionary with a content-hashed id, referenced by id where it recurs              |
| `@@#`  | injected context, raw (only with `--full-attachments`)                             |
| `$$>`  | the agent speaking                                                                 |
| `>>>`  | a tool call the agent made                                                         |
| `<<>`  | what a tool the agent called returned                                              |
| `**>`  | a mark that the agent thought; its reasoning text is never stored                  |
| `~~~`  | a divider: a mode change, a system event, or a compaction                          |

Flags: `--no-tools` collapses tool bodies to one marker, `--no-attachments` drops context records,
`--full-attachments` dumps them raw, `--truncate N` caps each tool body (`0` = no cap).

### ctxtrim.py pipeline

1. Split the conversation near the point that keeps `--keep` of it (by characters; default 0.5)
   verbatim. The split falls only at a human prompt, so a tool call and its result are never split.
2. Render the WHOLE conversation through transcript.py, with a marker line at the split.
3. Ask a headless agent for one structured, `/compact`-style summary of only the part ABOVE the
   marker; the part below is context it must not summarize. `--cli claude` uses `claude -p` on
   `--model`; `--cli glm` uses `claude-glm.cmd`, so each agent's history is summarized by itself.
   For GLM the instructions travel on stdin, because cmd.exe would mangle them as an argument.
4. Cut the older half off, keeping it: one `isCompactSummary` user record with no parent goes in
   just before the first kept prompt, which is re-pointed onto it. Every other line stays byte for
   byte (thinking signatures must survive), so the older half stays in the file as a log that
   resume never loads, and a later trim starts from the newest summary. The summary keeps the
   prompt's old parent in `trimParentUuid`, so a trim can be undone by hand. 15-10-2026
5. Rewrite the file in place, under its own id: written whole, then swapped in by one rename, and
   only when the records up to the split are still the ones summarized and resume, walking back
   from the last message, reaches the summary. `claude --resume <id>` continues it.

`--dry-run` prints the summary and writes nothing.

Run ctxtrim on a session no claude is writing to. To trim the session you are in, use cst: a
running Claude works from memory, so the trim only counts once it resumes, and cst makes it exit
first.


The game studio (claude-glm-coop)
---------------------------------

Two agents build a game together in `~/workspace/experiments/claude-glm-coop/game`: GLM (through
`claude-glm.cmd`) is product owner and QA and owns the ideas; Claude is the implementer. The owner
(Andrei) only steps in when something goes wrong.

| File (in claude-glm-coop/) | What it does                                                        |
|----------------------------|---------------------------------------------------------------------|
| `config.json`              | Every setting of the next run: rounds, fresh start or resume, idle  |
|                            | time, trim share, and per agent the model, effort, context window,  |
|                            | tools and role; plus the shared rules. `//` lines are comments.     |
| `run` / `run.ps1`          | Launch: a Game window plus the orchestrator. No arguments.          |
| `inject.ps1`               | Types a message into an agent's console (WriteConsoleInput), no     |
|                            | focus needed; logs to `inject.log`.                                 |
| `game/`                    | The game and its git repo; `CLAUDE.md` (shared rules, rewritten     |
|                            | each launch), `game.md` (the agents' living design), `.sessions`,   |
|                            | `.coop/`, `owner_note.txt`, logs and the context views.             |

How the orchestrator works:

- **Real agent windows.** Each agent is an interactive `claude` in its own console, opened by the
  orchestrator. Its role is passed with `--append-system-prompt-file`, so the role is part of the
  system prompt on every launch, resume and trim, and no summary can drop it.
- **Relay.** A turn is typed into an agent with `inject.ps1`; the text goes in first and Enter a
  moment later, so a large paste submits. Typing more than about 4 KB loses the beginning, so a
  message over 3,500 characters is written to `.coop/<agent>.msg.md` and only a one-line pointer
  to it is typed. The reply is read from the session `.jsonl`: the next
  assistant record whose `stop_reason` is `end_turn`.
- **Trim.** After each turn, an agent whose context (from the latest usage record) reaches
  `trim.at` of its window is trimmed by ctxtrim -- summarized by itself -- and only then stopped
  and relaunched on the trimmed session. When ctxtrim gives up, the agent keeps running
  untouched and the trim is retried 30 minutes later.
- **Recovery.** Only when an agent writes nothing for `idle_seconds`: a "continue" nudge, then a
  relaunch on the same session with the turn re-sent, then the loop stops.
- **Live config.** `config.json` is re-read at the start of every round: loop settings and shared
  rules apply at once; an agent's model, effort, tools and role apply at its next trim or relaunch
  (the trim summarizer already uses the new model). A file that does not parse is reported and the
  previous settings stay.
- **Keep-awake.** While the loop runs, Windows is asked not to sleep; the screen may still turn
  off.
- **Owner notes.** Three drop-boxes in `game/`: `owner_note.txt` reaches both agents,
  `owner_note.qa.txt` only GLM, `owner_note.implementer.txt` only Claude. A note is delivered at
  the start of the next round, marked as from the owner; a copy goes into `game/logs/` and the
  file is left empty for the next note. An empty file means no note.
- **Records.** `converse.log`, `loop.ndjson` (every turn), full-context renders of both sessions,
  `game.live` (the headless test after each round) and one git commit per round.
- **Stop / continue.** Closing the Game window stops everything; the session ids stay in
  `game/.sessions` and the next `./run` resumes them.

Never type into an agent window while the loop runs: a keystroke, Enter or Esc desyncs the relay.
Use `owner_note.txt` instead.


What we learned
---------------

About session files:

- A thinking block stores its `signature` but an **empty** `thinking` text: the reasoning is not
  persisted. A resume needs the signature intact, so kept assistant turns are copied byte for byte.
- Host compaction is three records: a `system` record `subtype: compact_boundary` with
  `parentUuid: null`, a `user` record `isCompactSummary: true`, and the last few turns re-attached.
- A hand-built history resumes and is used: a fact that lived only in a summary was recalled.
- "API Error: The response stopped arriving" is a `<synthetic>` record Claude Code writes when its
  stream goes dead -- in practice after the machine slept with an agent mid-turn.
- A running session works from memory: editing its file changes nothing for it, but the next
  `--resume` of the same id reads the edited file, and the session's later appends chain onto it.
- A `claude` started from inside another Claude session inherits `CLAUDE_CODE_CHILD_SESSION` and
  then saves no transcript at all. Start it with the `CLAUDE_CODE_*` variables removed.
- Windows can refuse, for a moment, to rename a file over a session (`Access is denied`): another
  program, most likely a virus scanner reading the file just written, holds one of them open.
  ctxtrim's in-place write retries the rename for about six seconds. (15-10-2026)
- After a trim, the next turn loads the CLAUDE.md files again: the first load lies in the part
  resume no longer reads, and Claude Code adds them back when the loaded history lacks them.
  (15-10-2026)

- On Opus 5.5, as on every model from Fable 5.1 on, the API checks a thinking block sent back
  against the history before it: when anything there changed since the block was made, that block
  and every later one are invalid, and claude -p has them dropped (unbilled) rather than refused.
  A trim's summary changes that history, so the kept half loses its thinking too: swarm's Reviewer
  went from 70% to 16%, not to about half (15-10-2026). Thinking may be removed oldest first, from
  the end, or all of it; a gap in the middle invalidates what follows it, and a removed block must
  not come back. https://platform.claude.com/docs/en/build-with-claude/preserved-thinking

About driving the agent consoles:

- Long typed bursts reach Claude Code in 2,048-character blocks; without the bracketed-paste
  markers, a window may keep only the last block. `inject.ps1` therefore sends every message as
  one paste (`-NoPaste` types plainly, for `/exit` or a shell command).
- The "trust this folder?" dialog puts "No, exit" first and preselected in this version, so a
  blind Enter closes the session. `inject.ps1 -Trust` reads the screen and answers Yes only when
  the dialog is shown; `-Dump` writes the visible screen text to the log.

About the half-keep trim in practice (implementer, 1M window, 08-10-2026):

- Two trims, at 60% then 63%, landed at ~24% and ~29%: the older half shrank to a 2-3k token
  summary, below the ~10% the design expected, so the landing is lower than 40-50%. Most of
  the gap is the kept half's thinking, which the summary invalidates (above; 15-10-2026).
- Continuity held both times: the first turns after each trim were on task with the build green,
  and the summary of a summary kept the role, the ground rules, the architecture and the owner's
  own directives. Old detail (build flags, early specs) dropped out but lives on disk.
- Compared to host `/compact`: far better continuity (hundreds of thousands of tokens of recent work
  kept verbatim), at the cost of larger contexts per turn and a thinner record of old history.


Permissions
-----------

Claude cannot grant itself these; the user adds them to a settings file.

| Operation                                 | Who needs it      | Where the rule lives          |
|-------------------------------------------|-------------------|-------------------------------|
| run ctxtrim, which writes into            | Claude, manually  | `homeauto/.claude/`           |
| `~/.claude/projects`                      |                   | `settings.local.json`         |
| `python` edits, `mv` (the exe-lock dodge) | the implementer   | its `tools` in `config.json`  |

The orchestrator's own trims run inside the process `./run` starts, so they need no rule. A rule in
`~/.claude/settings.json` applies to every session on the machine, the agents included -- prefer
project or per-agent scope.


Ideas and roadmap
-----------------

- **Summary size target.** Ask the summarizer for a size (say 30-60k tokens) so a trim keeps more
  old rationale and lands nearer 40-50%.
- **Host-visible trims.** Write a `compact_boundary` record so the host UI shows a trim as a
  compaction.
- **Images for GLM.** QA judges visuals through `--screenshot` PNGs; if the z.ai gateway cannot read
  images, the implementer's visual reports have to stand in.
