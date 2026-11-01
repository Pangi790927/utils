# proposals on the reMarkable

Proposals are reviewed by hand on a reMarkable Paper Pro, before anything is implemented. This
folder turns them into one PDF that is good to write on, and says how to get it onto the tablet and
back.

## What a proposal is

A proposal is one markdown file per step of a piece of work (`proposal_6a_grid_time.md`,
`proposal_7_fo8_config.md`, ...). It is written for review, as the step before the change lands in
the tree:

- **It presents the change.** It says what changes and why, what it knows it leaves as is ("known
  limits"), and what else could be done but isn't ("options, not in this code"). Its status line
  says where it stands: which revision, which earlier steps it needs, and whether it has been
  reviewed or applied.
- **It carries the complete diff.** The whole change, as a ```` ```diff ```` block against a named
  commit, applying with `patch -p1` from the repo root, in a stated order with the other proposals.
  Nothing is described without also being shown.
- **The code in it is tested.** The diff has been built and run (on the board, or with a stand-in
  where the board isn't available), and the proposal says so. Review is of a change that works,
  not of an idea.

Review happens on the document, not on the code: comments go next to the line they are about, and
the proposal is revised (r1, r2, ...) until it is accepted. Only then is it applied.

## Making the PDF

Needs XeLaTeX with `latexmk`, `fvextra`, `longtable`, `booktabs`, `enumitem`, `titlesec`, `fancyhdr`
and the DejaVu fonts (all in a normal TeX Live install). No pandoc: `md2tex.py` does the markdown
itself, for the subset proposals use.

```
cd /some/scratch/dir
python3 ~/workspace/utils/ai/remarkable/proposals/md2tex.py proposals.tex path/to/proposal_*.md
latexmk -xelatex -interaction=nonstopmode proposals.tex
```

The files go in the order given, so name them in the order they apply. The PDF:

- is 210×280 mm, the tablet's 3:4 shape, so it fills the screen;
- has the `.md` line number in the left margin: per paragraph, list item and table row, and on
  every line of a code block. A mark on the page maps back to an exact line of the file;
- has a 40 mm right margin, left empty for writing;
- has the file name in every page header;
- puts diff lines on coloured bands, for the Paper Pro's colour screen: `+` green, `-` red, `@@`
  blue, file headers grey and bold. Bands, not coloured text: on colour e-ink, thin coloured text
  looks black. Long code lines wrap with a ↪ marker.

## Uploading

The tablet on USB is at `10.11.99.1` (the PC is `10.11.99.2`). On the tablet, turn on
**Settings → Storage → USB web interface**. Check it is on with:

```
timeout 2 bash -c '</dev/tcp/10.11.99.1/80' && echo on
```

The PDF's file name becomes its name on the tablet, so copy it to a clear name first:

```
cp proposals.pdf "mon_fo2 proposals 6a-7.pdf"
curl -F "file=@mon_fo2 proposals 6a-7.pdf;type=application/pdf" http://10.11.99.1/upload
```

`{"status":"Upload successful"}` means it is there. It shows up at once, in the top-level folder.
A browser at `http://10.11.99.1` does the same by drag and drop.

## Getting it back

When the review is written, list the documents to find its ID, then download it. The PDF comes
back with the ink drawn on it:

```
curl -s -X POST http://10.11.99.1/documents/ \
    | python3 -c "import json,sys; [print(d['ID'], d['VissibleName']) for d in json.load(sys.stdin)]"
curl -o reviewed.pdf http://10.11.99.1/download/<ID>/pdf
```

(`VissibleName` is spelled that way by the tablet.) To hand it to Claude, render the pages and
let it read them:

```
pdftoppm -r 100 -png reviewed.pdf page
```

Handwriting is read, not parsed, so Claude first writes the comments back as a list matched to
file and line. That list is corrected before anything is changed in the proposals.

## Writing on it so it reads back well

- Write next to the line, or draw an arrow to it; the margin number does the rest.
- Strike out what should go and write the replacement nearby.
- **?** for a question, **✓** / **✗** for a decision.
