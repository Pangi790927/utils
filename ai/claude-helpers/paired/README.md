paired
======

A copy of the two-agent game studio's runner (as used in
`~/workspace/experiments/claude-glm-coop/`), without the game itself, kept so the setup can be
reinstated anywhere. 10-10-2026

| File          | What it is                                                                       |
|---------------|----------------------------------------------------------------------------------|
| `config.json` | Every setting of a run: rounds, fresh start or resume, idle time, trim share,    |
|               | and per agent the model, effort, context window, tools and role; plus the shared |
|               | rules written to `game/CLAUDE.md`. Lines starting with `//` are comments.        |
| `run`         | Git Bash launcher; delegates to `run.ps1`.                                       |
| `run.ps1`     | Opens the Game window and the orchestrator; stops everything when either closes. |
| `inject.ps1`  | Types into an agent's console (WriteConsoleInput, no focus needed): one          |
|               | bracketed paste by default, plainly with `-NoPaste`. `-Trust` answers the "trust |
|               | this folder?" dialog with Yes if shown; `-Dump` logs the visible screen text.    |

What else it needs, all in `utils/ai/claude-helpers/` (one level up):

- `gamedev_loop.py` -- the orchestrator. `run.ps1` finds it there by its full path.
- `ctxtrim.py` and `transcript.py` -- the half-keep trim and the session renderer.
- `game.md` -- the seed design, copied into `game/` only when none exists there.
- `claude-glm.cmd` on the PATH (in `~/workspace/claude-utils/`) -- GLM's launcher, holding the
  z.ai key. It is not copied here.

To reinstate:

1. Copy this folder to a new place, e.g. `~/workspace/experiments/<name>/`.
2. Edit `config.json`: the roles and shared rules describe the current game; rewrite them for a
   new one. Set `"fresh_start": true` for new sessions.
3. Run `./run` there. The orchestrator creates `game/` and its git repository, writes
   `game/CLAUDE.md`, seeds `game/game.md`, and opens both agent windows.

The game directory keeps `.sessions` (the two session ids), `.coop/` (relay files) and the owner
drop-boxes `owner_note.txt`, `owner_note.qa.txt`, `owner_note.implementer.txt`.
