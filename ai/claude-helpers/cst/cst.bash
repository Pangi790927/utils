# cst.bash -- Linux: brings a session trimmed by cst back in the terminal it was shown in.
# Sourced from ~/.bashrc: . ~/workspace/utils/ai/claude-helpers/cst/cst.bash
#
# Under Claude Code's daemon, cst can end a session (it types /exit through the session's relay)
# but can't type into the shell of the terminal that showed it. So this wraps `claude`: whenever
# it exits, `cst --take-request` tells whether cst ended a session started in this directory just
# now (waiting while cst rewrites it); if so, it starts `claude` again with the same arguments and
# `--resume <that id>`. Otherwise it returns at once, with claude's exit code. See cst.py.
# 14-10-2026

CST_PY="$HOME/workspace/utils/ai/claude-helpers/cst/cst.py"

claude() {
    local args=("$@")
    local rc sid
    while true; do
        command claude "${args[@]}"
        rc=$?
        sid=$(python3 "$CST_PY" --take-request "$PWD" 2>/dev/null) || return $rc
        [ -n "$sid" ] || return $rc

        # the same arguments, without an earlier resume/continue (a --resume without an id
        # opens the picker, so only an argument that isn't an option is taken as its id)
        local next=() i=0 a
        while [ $i -lt ${#args[@]} ]; do
            a=${args[$i]}
            i=$((i + 1))
            case "$a" in
                --resume|-r|--session-id)
                    if [ $i -lt ${#args[@]} ] && [ "${args[$i]#-}" = "${args[$i]}" ]; then
                        i=$((i + 1))
                    fi
                    continue ;;
                --resume=*|--session-id=*|--continue|-c)
                    continue ;;
            esac
            next+=("$a")
        done
        args=("${next[@]}" --resume "$sid")
        echo "cst: resuming the trimmed session $sid"
    done
}
