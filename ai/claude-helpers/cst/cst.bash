# cst.bash -- Linux: brings a Claude Code session that cst or cdt ended back in the terminal that
# showed it. Sourced from ~/.bashrc: . ~/workspace/utils/ai/claude-helpers/cst/cst.bash
#
# Nothing can type into another terminal on Linux, so cst and cdt can end a session but not start
# it again there. This function wraps `claude`: it runs claude with CST_SHELL set to this shell's
# pid, which tells cst the session can come back, and whenever claude exits it asks cst whether
# cst or cdt ended that session -- the one this claude showed, or the background one it attached
# to with `claude attach`. If so, it waits while the session is rewritten, then runs claude again:
# resuming the session with the same options (without a prompt given on the command line), or
# attaching to it again. Otherwise it returns at once, with claude's exit code. See cst.py.
# 14-10-2026, 24-10-2026-19:25

CST_PY="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/cst.py"

claude() {
    local args=("$@") again rc
    while true; do
        CST_SHELL=$$ command claude "${args[@]}"
        rc=$?
        mapfile -d '' -t again < <(python3 "$CST_PY" --take-request "$$" -- "${args[@]}")
        [ ${#again[@]} -gt 0 ] || return $rc
        args=("${again[@]}")
    done
}
