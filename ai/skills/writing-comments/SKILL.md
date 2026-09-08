---
name: writing-comments
description: Use when writing a new comment on a function, class or non-obvious block, when
  revising or shortening an existing one
  ("this comment is too long", "make it clearer","update this comment"), or when a comment must be
  rewritten because code moved or a reference went stale.
---

# Structure

## Brief

A comment is made out of @brief, one line, two at most, describing how the entity
fits the larger picture of the module being discussed, focusing on the action it allows. That module
can be the program, a library, a header, etc., depending on the target's scope. For example, read
inside unistd would have the brief: "Reads n bytes from the handler and stores it in a user buffer", or
something similar, either way, short, the action that the function does is described and only the
action that is relevant for the library, the context of the function, etc. A counter-example would be
"Reads n bytes from the handler and stores it in the user buffer after checking that the user buffer is
not null". The last part of this example is a detail. So the first step is to separate detail from
important/structural.

## Core and detail

The second part of each comment will be the description itself; this description should be split into
two: core, the principle of the commented target, what matters about it and the reason it exists. As such, the
core links this comment with other core principles around it; it embeds the object into the behaviour of
the module. It must be a succinct pass over the important mechanism of the function (those mechanisms
are the ones from "split description into detail and core"). It is also the promise of the object, for
example, read would contain things like: "Waits for a message to arrive", "Is atomic with respect to
other operations on the file descriptor", etc. Those are behaviours of the object "read" that will
not change, else the object loses its meaning. The user should be asked if the model can't figure
out what is core and what is detail.

The detail part will contain (if any) quirks: behaviours that may change because the object is
not interested in them. Those details can change inside the implementation and the object's role
could still be achieved just as well. Edge cases and strange input conditions go into detail too.

## Parameters and returns (optional)

The next section after establishing the core and detail section will be the input and output section.
If the previous section explained how the target works, this section will explain to you what it expects
to receive and what it will output to give back to you. Any such interface should be documented if
the object has it, params, return types, template parameters, exposed fields that the user is expected
to mingle with, etc.
This section makes sense only for some objects; comments for objects that have no interface should
leave it out.

## Notes, examples (optional)

If there are things that are unexpected for someone using this code, reading this comment, or if an
example makes things easier, then this section should be included in the comment. Those should not
be included in the core, and the reverse holds too: if something is core, it can't be a note.

## Size

A comment is always written in full first and shrunk afterwards, never composed straight to the
target size. Writing it whole is what forces the research to happen: the code has to be read
closely enough to say what belongs in every section, and core has to be separated from detail
before there is anything to give up. A comment composed directly at a small size skips both steps,
and what comes out looks like a short comment while being an unresearched one, because the sections
that were never written cannot be told apart from the sections that were considered and cut.

Should a size be specified for the comment, the sections give way in a fixed order: notes first,
then detail, then parameters, then core. Each one is shrunk, and dropped entirely if that is not
enough, before the next is touched, so the cuts land on the least essential part of the comment
first. The brief is never dropped: in some form, however short, it is the minimum a comment can be.

A section is given up whole rather than thinned past the point where it still says something. Half
a note explains nothing, and a detail with its reason cut out is worse than no detail at all, since
it names a mechanism without saying what happens if it changes.

Cuts reaching the core are a signal, not merely a cost. If a size forces something out of core,
either the size asked for is wrong for this target, or what was sitting in core was never core to
begin with. Both are worth saying out loud rather than quietly obeying the number.

# Process

## Check the comment against the code

Before a comment is considered done it should be checked against what the code actually does, not
against what the previous comment said. List the things the target does that can be observed from
outside it: what it changes, what it gives back, what it refuses to do, what it leaves behind.
Every item on that list should then either appear in one of the sections above, or be left out on
purpose. A comment that describes three of five behaviours is worse than one that describes none,
because it reads as complete and so it stops the reader from looking any further.

This check is also where the core and detail split gets tested. If an observed behaviour finds no
section that wants it, it was usually misfiled: the caller can see it, so it is not a detail, and if
it is not a promise either then it belongs in parameters or in notes.

## Revising an existing comment

An existing comment should not be edited in place. Prose keeps its shape when it is edited, and the
shape is usually what is wrong with it, so trimming words gives a smaller comment with the same
defect. The facts should be taken out of it first: list what the old comment knows, one item per
fact, ignoring the order they were written in. Duplicates collapse, and a surprising number of long
comments say the same thing twice in different words.

Each fact is then kept or dropped. Anything the reader can get from the code itself goes. A story
about how something was discovered is reduced to the finding and its date. A reference to something
outside this repository is either restated in general terms or removed, since the reader cannot
follow it. What survives is sorted into brief, core, detail, parameters and notes as if the comment
were being written for the first time, and then checked against the code as above.

Whatever was dropped should be reported to whoever asked for the revision. Shrinking a comment
throws knowledge away quietly, and that is the one real risk in doing it.

# Helpers

## Split description into detail and core

The general description of everything that a target object does (function, structure, etc.) can be
further split into detail and core. Core ideas are the ones that are important for what that object
does; the object can't exist without them, or its use would be altered. Detail is a means to reach
an end: details can change, the core idea should not.

# Example

`examples/comments/force_release.md`, kept beside this skill in the same plugin, walks the structure
through a real revision: the function, two failed rewrites, and the sorted result, with the
reasoning behind every core/detail call.
