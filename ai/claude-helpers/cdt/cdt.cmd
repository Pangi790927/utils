@echo off
rem cdt -- drop the reasoning from a Claude Code session, in place; run from one, it resumes it.
rem Usage, inside a session: ! cdt [--dry]    or, for a closed one: cdt SESSION.jsonl or ID [--dry]
rem 15-10-2026
python "%USERPROFILE%\workspace\utils\ai\claude-helpers\cdt\cdt.py" %*
