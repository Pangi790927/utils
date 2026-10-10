Console RTS -- the claude-glm-coop game
=======================================

Status: under discussion, not yet launched. This is the living design for the game the two-model
loop builds. The loop itself (GLM = QA / product owner, Claude Opus 5.5 = implementer, the three
live windows, the logs) is `gamedev_loop.py` + `run` / `run.ps1`; see the tools table in desc.md.
The game is built in `~/workspace/experiments/claude-glm-coop/game`.


The game
--------

A console REAL-TIME STRATEGY game (not turn-based), in the style of Red Alert 2 (RA2): a
base-building RTS where you LOSE when all of your buildings are destroyed.

- A large scrollable 2D tile map -- target around 512x512 characters -- so camera scrolling is
  central, with a MINIMAP showing building locations.
- Units and buildings act in real time on a tick loop (colib.h coroutines for the loop and timing).
  Run as fast as the terminal allows -- terminals are slow, so speed matters; make them faster if
  you can, and aim for RA2-like pacing.
- Interactive input: MOUSE click to select a unit and to issue move / attack orders; KEYBINDS for
  commands; camera scroll by BOTH screen-EDGE mouse position AND WASD.
- COLOURED terminal rendering -- factions, terrain, selection and bullets each in colour (ANSI
  escape codes, or the Windows console colour API).
- Interactivity in RA2's tiers: a thing that matters is COLOURED to stand out; a thing you can act
  on is CLICKABLE; and a thing you should click is HIGHLIGHTED (e.g. a lighter background) so you
  can see where to click.
- Windows input through the console API (enable mouse input, ReadConsoleInput for mouse + key
  events); interactive mouse needs a real console window.
- The game CORE (map, units, orders, tick update) stays separate from the input / render layer, so
  the headless `run_test()` drives the same core with a scripted sequence of synthetic input events
  and asserts the resulting state -- no real mouse, keyboard or console needed.

Build / test: a virt_composer C++/Lua project, MSVC `cl` via `make`, headless `main.exe --test`
(the math_writer / simulator pattern). The full recipe is in the loop's briefs.


Sprites -- starting ideas
-------------------------

These are seeds. The agents are encouraged to improve them, or invent nicer models, and to use
colour throughout.

Tank (directional -- the five frames are the turret / barrel pointing different ways):

    # | #
    # O #
    #   #

    #\  #
    # O #
    #   #

    #   #
    #-O #
    #   #

    #   #
    # O #
    #   #

    ##|##
      O
    #####

Soldier, firing bullets:

      .

     O|

    .   _
        O

War factory (units exit at the `U` gate):

    |-----UUUUU-----|
    |               |
    |               |
    |    WFactory   |
    |               |
    |               |
    |_______________|


Decisions (08-10-2026)
----------------------

- Feel: Red Alert 2. Lose condition: all of your buildings destroyed.
- Map: around 512x512 characters; scrolling is central; a minimap shows building locations.
- Tempo: as fast as the terminal allows; faster is better; RA2-like pacing.
- Rendering: use MULTI-CELL sprites wherever they look better. ("Multi-cell" means a unit or
  building is drawn as a small grid of characters across several terminal cells -- like the 3-line
  tank and the war-factory box above -- rather than one glyph. It looks better but makes the tile
  grid coarser, since each unit takes several cells; the agents choose the balance.)
- Interactivity: RA2 tiers -- coloured to stand out, clickable to act on, highlighted (a lighter
  background) to show where to click.
- The agents read ./game.md (copied into the game directory at launch; their briefs point at it)
  and keep their set roles. BOTH work as a single instance each -- no spawning or delegating to
  sub-agents.

Left to the agents (sensible defaults, improve over rounds)
-----------------------------------------------------------

- The unit roster beyond tank, soldier and war factory (RA2 flavour: harvester, refinery, power,
  turret, ... -- add over rounds).
- Terrain types and whether they block movement or sight.
- Exact tick rate and unit speeds; ranged (bullets) vs melee mix.
- The colour palette per faction, terrain, selection and projectiles.
- Nicer sprites than the seeds above.
