# ai

Everything AI-related, kept in the same repo as the work it came from. Not a comment project and
not tied to any one subject: conventions, tooling, templates and records of real sessions all live
here, whatever they happen to be about.

It is shaped as a Claude Code plugin so it travels. Install it on any machine with:

```
/plugin marketplace add Pangi790927/utils
/plugin install pangi-ai@pangi-utils
```

An edit here reaches every machine with a `git pull`, instead of files being copied into
`~/.claude/` one machine at a time. The marketplace manifest is at the repo root, in
`.claude-plugin/marketplace.json`; this directory is the plugin itself.

## The three directories

The split is by **how a file is used**, never by what it is about. Any subject can appear in all
three at once, and the first one to arrive already does: the comment conventions are a skill, a
blank comment skeleton would be a template, and one real comment rewritten four times is an
example.

### `skills/`

Instructions Claude follows on its own. A skill is a folder with a `SKILL.md`, pulled in only when
its `description` matches the task at hand, so it costs nothing until it is relevant. This is where
a convention lives once it has settled: how something is done, every time, without being asked.

Written for the model rather than for a reader. If a file would only ever be read by a person, it
belongs in one of the other two directories.

### `templates/`

Files meant to be copied into a project and filled in. Generic on purpose - a template that names a
real function has failed, because the next project does not have that function. Blanks, headings
and prompts, with the decisions left open.

### `examples/`

Real cases, kept because they happened. Specific on purpose: they name actual files and symbols,
carry the wrong turns as well as the result, and are read to see how a rule behaves under load.
Never copied, never filled in.

## Where a new file goes

- Something Claude should do unprompted, every time? → `skills/`
- Something a person copies and edits? → `templates/`
- The record of one real case, with its reasoning? → `examples/`

When something is both a rule and the story of where the rule came from, split it: the rule into a
skill, the story into `examples/`, and the skill points at it.

## What else can live here

A plugin can carry more than skills, and this one is expected to grow into the rest of it as the
need appears - slash commands in `commands/`, subagent definitions in `agents/`, hooks that the
harness enforces rather than requests, an `.mcp.json` for servers this work depends on. Those are
plugin-native directories at this level, alongside the three above.

Anything AI-related that is not one of those is still welcome here; give it its own directory and
add a line to this file saying what it is and how it is meant to be used.

## Contents so far

- `skills/writing-comments` - the structure of a comment (brief, core, detail, parameters, notes),
  how it is checked against the code, how it is revised, and what is dropped first when a size is
  asked for.
- `templates/claude_md/` - CLAUDE.md starting points: minimal, header-first C++ library, and an
  acknowledgment of the Three Laws.
- `templates/project_init/` - setting a repo up for AI work, and starting a header-first C++
  project.
- `examples/comments/force_release.md` - one comment in `virt_composer.cpp`, rewritten four times.
