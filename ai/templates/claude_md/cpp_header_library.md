<!-- For a header-first C++ repo in the style of utils: many independent headers, few or no
     build artefacts, examples rather than a framework. -->

# CLAUDE.md

## What this is

A collection of independent C++ headers. Each one stands alone and can be dropped into another
project by itself; nothing here is a framework, and no header may require the others unless it says
so at the top.

## Layout

- `*.h` at the root are the library itself, one subject per file.
- `tests/` holds what can be run; `*.example` files are illustrations, not built by default.
- <subdirectories, if any, and what makes something belong in one>

## Building

```
<compiler invocation or makefile target for the tests>
```

There is no install step and no package. A user copies the header.

## Conventions

- A header must compile on its own, included first in a translation unit.
- <namespace, naming, include-guard convention>
- <what depends on what: which headers are allowed to include which>
- Comments follow the `writing-comments` skill: brief, core, detail, parameters, notes.

## Things that look wrong and are not

- <deliberate duplication between headers that keeps them independent>
