# Starting a header-first C++ project

The shape used by this repo: independent headers, no build system to speak of, examples instead of
a framework.

## Files

```
<name>.h              the header itself, self-contained, compiles when included first
tests/<name>_test.cpp what proves it works
<name>.example        illustration, not built by default
CLAUDE.md             from ../claude_md/cpp_header_library.md
makefile.example      a build line someone can copy, not a build system
```

## Rules worth deciding on day one

- **Independence.** May a header include another header from this repo? Decide once, write it in
  CLAUDE.md, and prefer duplication over a dependency you did not intend to promise.
- **Include guards.** `#pragma once` or a spelled-out guard, and the naming for it.
- **Namespace.** One per repo, or one per header.
- **What is public.** Everything in a header is reachable, so mark what is not meant to be used -
  a nested `detail` namespace, or an `[INTERNAL]` tag on the comment.
- **Comments.** Adopt the `writing-comments` skill early. Retrofitting comment structure onto a
  finished header is far more work than writing it that way from the start.

## First commit

A header that does one useful thing, its test, and the CLAUDE.md. Not a skeleton of empty files:
the first real function is what settles the conventions above, and arguing about them in the
abstract beforehand rarely survives contact with the code.
