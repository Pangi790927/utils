@echo off
rem cst -- trim the Claude Code session you run it from, in place, and resume it by itself.
rem Usage, inside a session: ! cst [--model M] [--keep 0.5]
rem 10-10-2026
python "%USERPROFILE%\workspace\utils\ai\claude-helpers\cst\cst.py" %*
