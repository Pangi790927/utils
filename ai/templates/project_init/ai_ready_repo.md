# Making an existing repo pleasant to work on with Claude

In order, cheapest first. Each step is optional on its own; the first two carry most of the value.

## 1. CLAUDE.md at the root

Read at the start of every session in this repo, so it costs context every time. Keep it to what
cannot be worked out by reading the code: the build command, the conventions, the deliberate
oddities. Start from `../claude_md/minimal.md`.

What does *not* belong in it: anything derivable from the code, a file-by-file tour, history that
git already records, or a rule that only applies to one kind of task. The last of these is a skill.

## 2. Skills for the repeated tasks

A skill is a folder with a `SKILL.md`, and unlike CLAUDE.md it is only loaded when its description
matches the task at hand. Anything that is a procedure rather than a fact belongs here: a release
checklist, a review pass, the conventions for one subsystem.

- `.claude/skills/<name>/SKILL.md` for skills that belong to this repo and are committed with it.
- a plugin, as in `utils/ai`, for conventions that should follow you between repos.

The description is what decides whether the skill is ever used, so write it as the situations that
should trigger it, in the words someone would actually say.

## 3. Permissions, so the session stops asking

`.claude/settings.json` can allow the read-only commands this project uses constantly - the build,
the test runner, the formatter - so they no longer prompt. Committed, this applies to everyone
working on the repo; `.claude/settings.local.json` is the uncommitted, personal version.

## 4. Hooks, for what must happen every time

Anything phrased as "always do X after Y" belongs in a hook, not in a document: a document is a
request, a hook is enforced by the harness. Formatting after an edit is the usual first one.

## 5. Templates and examples

If the repo has a shape that new files are expected to follow, keep one canonical example and point
at it from CLAUDE.md. It is read far more reliably than a description of the shape.
