# statusline.sh -- the Claude Code status line of Windows: the context in use, and what cst or cdt
# is doing to the session. Claude Code runs it in Git Bash with the status line input JSON on
# stdin, and it prints one colored line:
#
#   60k/1.0M (6%)                       the context in use, as of the last reply: blue under 60%,
#                                       yellow under 85%, red above; grey "context n/a" before it
#   cst: summarizing… 0:32              cst or cdt works on this session, at that step so far
#   cst: trimmed, waiting for a reply   the session came back, and has not answered since
#   cst: nothing changed, see cst.log   cst or cdt stopped without changing it; for a minute
#
# It reads the status the helper leaves (cst/common.py, set_status) itself, and starts Python
# (running.py) only while that status says the helper runs, to tell whether it still does:
# starting Python every second made typing in Claude Code freeze now and then (desc.md). The
# helper removes a "back" status at the session's first reply (cst/driver_windows.py, after_back);
# this drops one after BACK_SHOW_S, which only a helper that died leaves, and a "failed" one after
# FAILED_SHOW_S. install.py beside it puts `bash <this file>` into the Claude Code settings; Linux
# has statusline/statusline.py instead. 28-10-2026-10:45, 28-10-2026-11:35

BACK_SHOW_S=1800        # as BACK_SHOW_S in cst/driver_windows.py
FAILED_SHOW_S=60
BLUE="01;34"
YELLOW="01;33"
RED="01;31"
GREY="01;30"
CYAN="01;36"

# Sets REPLY to the value of the first "$1" in the JSON text $2 -- a string's contents, or a
# number -- or to nothing when there is none, or it is null. 28-10-2026-10:45
value() {
    local re="\"$1\"[[:space:]]*:[[:space:]]*(\"([^\"]*)\"|(-?[0-9][0-9.eE+-]*))"
    REPLY=
    if [[ $2 =~ $re ]]; then
        REPLY=${BASH_REMATCH[2]}${BASH_REMATCH[3]}
    fi
}

# Prints the text $2 in the SGR color $1 as the status line, and ends. 28-10-2026-10:45
show() {
    printf '\033[%sm%s\033[00m' "$1" "$2"
    exit 0
}

# Sets REPLY to the token count $1 the short way: 1.2M, 342k, or 512. 28-10-2026-10:45
short() {
    local n=${1%%.*}
    if (( n >= 1000000 )); then
        REPLY="$(( n / 1000000 )).$(( n % 1000000 / 100000 ))M"
    elif (( n >= 1000 )); then
        REPLY="$(( n / 1000 ))k"
    else
        REPLY=$n
    fi
}

# Shows the state the cst or cdt helper left for session $1, if it has one to show: "running"
# through running.py, given the input $2, and "back" and "failed" while they last. A status that
# has lasted its time is removed. 28-10-2026-10:45, 28-10-2026-11:35
helper_state() {
    local run="${LOCALAPPDATA//\\//}/cst" st state what text since now line
    [[ $1 =~ ^[A-Za-z0-9._-]+$ && -f $run/status.$1 ]] || return
    IFS= read -r -d '' st 2>/dev/null < "$run/status.$1"
    value state "$st"; state=$REPLY
    value what "$st"; what=${REPLY:-cst}
    value text "$st"; text=$REPLY
    value since "$st"; since=${REPLY%%.*}
    printf -v now '%(%s)T' -1
    case $state in
        running)
            line=$(printf '%s' "$2" | python "$HERE/running.py")
            [[ $line ]] && { printf '%s' "$line"; exit 0; }
            return ;;
        back)
            if (( now - ${since:-0} < BACK_SHOW_S )); then
                show "$CYAN" "$what: $text, waiting for a reply"
            fi ;;
        failed)
            if (( now - ${since:-0} < FAILED_SHOW_S )); then
                show "$RED" "$what: $text"
            fi ;;
    esac
    rm -f "$run/status.$1"
}

HERE=${BASH_SOURCE[0]%/*}
[[ $HERE == "${BASH_SOURCE[0]}" ]] && HERE=.
IFS= read -r -d '' input

value session_id "$input"
helper_state "$REPLY" "$input"

# The context numbers are read inside "context_window" only: other parts of the input, such as
# "rate_limits", have a "used_percentage" of their own. 28-10-2026-10:45
re='"context_window"[[:space:]]*:[[:space:]]*(\{([^{}]|\{[^{}]*\})*\})'
cw=
[[ $input =~ $re ]] && cw=${BASH_REMATCH[1]}
value used_percentage "$cw"; pct=$REPLY
value total_input_tokens "$cw"; used=$REPLY
value context_window_size "$cw"; size=$REPLY
if [[ -z $pct || -z $size || ${size%%.*} == 0 ]]; then
    show "$GREY" "context n/a"
fi
if (( ${pct%%.*} < 60 )); then
    color=$BLUE
elif (( ${pct%%.*} < 85 )); then
    color=$YELLOW
else
    color=$RED
fi
short "${used:-0}"; used=$REPLY
short "$size"; size=$REPLY
show "$color" "$used/$size ($pct%)"
