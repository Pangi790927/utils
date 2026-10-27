# cst.bash -- Linux: brings a Claude Code session that cst or cdt ended back in the terminal that
# showed it. Sourced from ~/.bashrc: . ~/workspace/utils/ai/claude-helpers/cst/cst.bash. Sourced
# by hand in a shell already open, it works there too, even for a claude already running in it:
# Ctrl+Z, source this file, `fg`.
#
# Nothing can type into another terminal on Linux, so cst and cdt can end a session but not start
# it again there; the terminal's shell can. Sourcing this file leaves a marker naming this shell,
# which tells cst that the session can come back, and hooks the prompt. When cst or cdt end a
# claude this shell runs -- the session it shows, or the background one it attached to with
# `claude attach` -- they leave a request for this shell. Once that claude has exited, the hook
# waits while the session is rewritten, then runs claude again as the request says: resuming the
# session with the same options (without a prompt given on the command line), or attaching to it
# again. See cst.py.
# 14-10-2026, 24-10-2026-19:25, 27-10-2026-18:40

CST_PY="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/cst.py"
CST_RUN="${XDG_RUNTIME_DIR:-/tmp/cst-$UID}/cst"

# Runs claude again while a request is left for this shell. It loops because a claude run from
# here that cst or cdt end again leaves its request before this hook returns, and the prompt shown
# then runs no hook. The exit status the prompt sees stays the one before the hook.
# 27-10-2026-18:40
__cst_prompt() {
    local rc=$?
    local again
    while [ -e "$CST_RUN/request.$$" ]; do
        mapfile -d '' -t again < <(python3 "$CST_PY" --take-request "$$")
        if [ ${#again[@]} -eq 0 ]; then
            break
        fi
        command claude "${again[@]}"
    done
    return $rc
}

# The marker holds this shell's start time, as /proc gives it, so that it names this very shell
# and never a later process given the same pid. 27-10-2026-18:40
mkdir -p "$CST_RUN"
read -r -a __cst_stat < /proc/$$/stat
echo "${__cst_stat[21]}" > "$CST_RUN/shell.$$"
unset __cst_stat

# The `claude` wrapper of the cst.bash before 27-10-2026 is still defined in a shell that loaded
# it; the hook takes its place. 27-10-2026-18:40
if declare -f claude | grep -q CST_SHELL; then
    unset -f claude
fi

case ";$PROMPT_COMMAND;" in
    *";__cst_prompt;"*)
        ;;
    *)
        PROMPT_COMMAND="__cst_prompt${PROMPT_COMMAND:+;$PROMPT_COMMAND}"
        ;;
esac
