#!/usr/bin/env python3
"""Turns proposal markdown files into one LaTeX document for reviewing on a reMarkable.

Every block carries its markdown source line number in the left margin (code blocks: every line),
so a handwritten note can be mapped back to the .md line. The right margin is left wide for notes.
Each file starts on a new page, with its name in the page header.

Handles what proposals use: headings, paragraphs, nested lists, pipe tables, `code`, **bold**,
*italic* and fenced code blocks (```diff ones coloured). Builds with xelatex (DejaVu fonts).

Usage: md2tex.py out.tex file.md [file.md ...]
See README.md next to this file for the whole round trip.
"""
import os
import re
import sys

PREAMBLE = r"""\documentclass[10pt]{article}
\usepackage[paperwidth=210mm,paperheight=280mm,left=20mm,right=40mm,top=18mm,bottom=16mm,
            headsep=5mm,footskip=8mm]{geometry}
\usepackage{fontspec}
\setmainfont{DejaVu Serif}
\setsansfont{DejaVu Sans}
\setmonofont{DejaVu Sans Mono}[Scale=0.95]
\usepackage{xcolor}
\usepackage{fvextra}
\usepackage{longtable}
\usepackage{array}
\usepackage{booktabs}
\usepackage{enumitem}
\usepackage{fancyhdr}
\usepackage{titlesec}
\usepackage[hidelinks]{hyperref}

% diff bands: saturated enough to still read as colour on e-ink, light enough for black text
\definecolor{addbg}{HTML}{8EE08E}
\definecolor{delbg}{HTML}{F29A9A}
\definecolor{hunkbg}{HTML}{9CC4F2}
\definecolor{filebg}{HTML}{D0D0D0}
\definecolor{numc}{HTML}{808080}
% the band colour of the code line being set: code_block() defines mdbg:<line> for each band
\newcommand{\mdbgcolor}{\csname mdbg:\number\value{FancyVerbLine}\endcsname}

\setlength{\parindent}{0pt}
\setlength{\parskip}{4pt}
\linespread{1.15}
\setlist{nosep,leftmargin=1.4em}
\setlist[itemize]{label=--,topsep=2pt,itemsep=2pt}

\pagestyle{fancy}
\fancyhf{}
\fancyhead[L]{\sffamily\small\leftmark}
\fancyhead[R]{\sffamily\small\thepage}
\renewcommand{\headrulewidth}{0.3pt}

\titleformat{\section}{\sffamily\Large\bfseries}{}{0pt}{}
\titleformat{\subsection}{\sffamily\large\bfseries}{}{0pt}{}
\titleformat{\subsubsection}{\sffamily\normalsize\bfseries}{}{0pt}{}
\titlespacing*{\subsection}{0pt}{10pt}{3pt}

% the markdown source line, in the left margin
\newcommand{\srcln}[1]{\leavevmode\llap{\textcolor{numc}{\sffamily\scriptsize #1}\hspace{7mm}}}
\newcommand{\BS}{\textbackslash}
\renewcommand{\theFancyVerbLine}{\textcolor{numc}{\sffamily\scriptsize\arabic{FancyVerbLine}}}

\begin{document}
"""

INLINE_BREAK_AFTER = set("_/.,->(:=*")


def esc_text(s):
    out = []
    for ch in s:
        if ch == "\\":
            out.append(r"\textbackslash{}")
        elif ch in "{}$&#%_":
            out.append("\\" + ch)
        elif ch == "^":
            out.append(r"\^{}")
        elif ch == "~":
            out.append(r"\textasciitilde{}")
        else:
            out.append(ch)
    return "".join(out)


def esc_code_span(s):
    out = []
    for ch in s:
        out.append(esc_text(ch))
        if ch in INLINE_BREAK_AFTER:
            out.append(r"\allowbreak{}")
    return r"\texttt{" + "".join(out) + "}"


def inline(s):
    """Code spans are set aside first, so bold/italic markers around them still pair up."""
    spans = []

    def keep(m):
        spans.append(esc_code_span(m.group(1)))
        return "\x00%d\x00" % (len(spans) - 1)

    t = re.sub(r"`([^`]*)`", keep, s)
    t = esc_text(t)
    t = re.sub(r"\*\*(.+?)\*\*", r"\\textbf{\1}", t)
    t = re.sub(r"(?<![\w*])\*(?!\s)(.+?)(?<!\s)\*(?![\w*])", r"\\emph{\1}", t)
    return re.sub("\x00(\\d+)\x00", lambda m: spans[int(m.group(1))], t)


def esc_verb(s):
    """A code line for Verbatim with commandchars=\\\\\\{\\}."""
    out = []
    for ch in s:
        if ch == "\\":
            out.append(r"\BS{}")
        elif ch in "{}":
            out.append("\\" + ch)
        else:
            out.append(ch)
    return "".join(out)


def diff_kind(line):
    if line.startswith("+++") or line.startswith("---"):
        return "file"
    if line.startswith("+"):
        return "add"
    if line.startswith("-"):
        return "del"
    if line.startswith("@@"):
        return "hunk"
    return None


def code_block(lines, first_no, lang):
    """Diff lines other than context sit on a coloured band (fvextra's line highlighting, its colour
    picked per line by \\mdbgcolor): the tablet's colour e-ink shows a band, while thin coloured
    text there looks black."""
    marked = []
    body = []
    for k, line in enumerate(lines):
        line = line.expandtabs(4)
        no = first_no + k
        kind = diff_kind(line) if lang == "diff" else None
        tex = esc_verb(line)
        if kind:
            marked.append((no, kind))
        if kind == "file":
            tex = r"\bfseries{}" + tex
        body.append(tex)

    out = []
    for no, kind in marked:
        out.append(r"\expandafter\def\csname mdbg:%d\endcsname{%sbg}%%" % (no, kind))
    out.append(
        r"\begin{Verbatim}[commandchars=\\\{\},fontsize=\footnotesize,breaklines,"
        r"breakanywhere,numbers=left,numbersep=5mm,firstnumber=" + str(first_no)
        + ",highlightcolor=\\mdbgcolor,highlightlines={"
        + ",".join(str(no) for no, _ in marked) + "}]"
    )
    out += body
    out.append(r"\end{Verbatim}")
    return "\n".join(out)


def table_block(rows):
    """rows: list of (line_no, cells); the second row is the |---| separator."""
    header_no, header = rows[0]
    body = rows[2:]
    ncols = len(header)
    lens = [1] * ncols
    for _, cells in [rows[0]] + body:
        for i, c in enumerate(cells[:ncols]):
            lens[i] = max(lens[i], min(len(c), 60))
    total = sum(lens)
    widths = [max(0.12, l / total) for l in lens]
    scale = sum(widths)
    spec = "".join(
        r">{\raggedright\arraybackslash}p{%.3f\linewidth}" % (w / scale - 0.025) for w in widths
    )
    out = [r"{\small", r"\begin{longtable}{" + spec + "}", r"\toprule"]
    if any(c.strip() for c in header):
        cells = [r"\textbf{" + inline(c) + "}" for c in header]
        cells[0] = r"\srcln{" + str(header_no) + "}" + cells[0]
        out.append(" & ".join(cells) + r" \\ \midrule")
    for no, cells in body:
        cells = (cells + [""] * ncols)[:ncols]
        tex = [inline(c) for c in cells]
        tex[0] = r"\srcln{" + str(no) + "}" + tex[0]
        out.append(" & ".join(tex) + r" \\")
    out.append(r"\bottomrule")
    out.append(r"\end{longtable}}")
    return "\n".join(out)


def split_row(line):
    s = line.strip()
    if s.startswith("|"):
        s = s[1:]
    if s.endswith("|"):
        s = s[:-1]
    return [c.strip() for c in s.split("|")]


LIST_RE = re.compile(r"^(\s*)([-*]|\d+[.)])\s+(.*)$")


def list_block(items):
    """items: list of (line_no, indent, ordered, text)."""
    out = []
    stack = []
    for no, indent, ordered, text in items:
        while stack and indent < stack[-1][0]:
            out.append(r"\end{%s}" % stack.pop()[1])
        if not stack or indent > stack[-1][0]:
            env = "enumerate" if ordered else "itemize"
            out.append(r"\begin{%s}" % env)
            stack.append((indent, env))
        out.append(r"\item \srcln{" + str(no) + "}" + inline(text))
    while stack:
        out.append(r"\end{%s}" % stack.pop()[1])
    return "\n".join(out)


def convert(path):
    with open(path, encoding="utf-8") as f:
        lines = f.read().split("\n")
    name = os.path.basename(path)
    out = [r"\clearpage", r"\markboth{" + esc_text(name) + "}{}"]
    i = 0
    n = len(lines)
    while i < n:
        line = lines[i]
        no = i + 1
        if not line.strip():
            i += 1
            continue

        m = re.match(r"^```\s*(\S*)", line)
        if m:
            lang = m.group(1)
            j = i + 1
            while j < n and not lines[j].startswith("```"):
                j += 1
            out.append(code_block(lines[i + 1 : j], i + 2, lang))
            i = j + 1
            continue

        m = re.match(r"^(#{1,4})\s+(.*)$", line)
        if m:
            level = len(m.group(1))
            text = inline(m.group(2))
            cmd = ["section*", "subsection*", "subsubsection*", "paragraph*"][level - 1]
            out.append("\\" + cmd + r"{\srcln{" + str(no) + "}" + text + "}")
            i += 1
            continue

        if re.match(r"^\s*(-{3,}|\*{3,})\s*$", line):
            out.append(r"\par\noindent\rule{\linewidth}{0.3pt}\par")
            i += 1
            continue

        if line.lstrip().startswith("|"):
            rows = []
            while i < n and lines[i].lstrip().startswith("|"):
                rows.append((i + 1, split_row(lines[i])))
                i += 1
            out.append(table_block(rows))
            continue

        if LIST_RE.match(line):
            items = []
            while i < n and lines[i].strip():
                lm = LIST_RE.match(lines[i])
                if lm:
                    ordered = lm.group(2)[0].isdigit()
                    items.append([i + 1, len(lm.group(1)), ordered, lm.group(3)])
                elif lines[i].startswith(" ") and items:
                    items[-1][3] += " " + lines[i].strip()
                else:
                    break
                i += 1
            out.append(list_block(items))
            continue

        para = [line.strip()]
        i += 1
        while i < n and lines[i].strip():
            nxt = lines[i]
            if (
                nxt.startswith("```")
                or nxt.startswith("#")
                or nxt.lstrip().startswith("|")
                or LIST_RE.match(nxt)
            ):
                break
            para.append(nxt.strip())
            i += 1
        out.append(r"\srcln{" + str(no) + "}" + inline(" ".join(para)) + r"\par")
    return "\n\n".join(out)


def main():
    out_path = sys.argv[1]
    parts = [PREAMBLE]
    for path in sys.argv[2:]:
        parts.append(convert(path))
    parts.append(r"\end{document}")
    with open(out_path, "w", encoding="utf-8") as f:
        f.write("\n\n".join(parts) + "\n")


if __name__ == "__main__":
    main()
