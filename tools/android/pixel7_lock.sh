#!/bin/bash
# Pixel 7 sharing between Claude sessions on Marko's Mac (DJ Mantra app tests
# and the NDI camera app tests use the same phone). Installed on the Mac as
# ~/pixel7/pixel7-lock.sh; the state lives in ~/pixel7/. bash 3.2-safe.
#
#   pixel7-lock.sh status                    who has the phone (exit 0 free, 1 busy)
#   pixel7-lock.sh take <owner> <minutes> <what>   take it (exit 0) or say who has it (exit 1)
#   pixel7-lock.sh wait <owner> <minutes> <what>   wait until free, then take it
#   pixel7-lock.sh extend <owner> <minutes>  keep it longer (only the owner)
#   pixel7-lock.sh give <owner>              give it back (only the owner)
#
# owner: "djmantra" or "camera". A lock whose time ran out more than 10 minutes
# ago is stale: the next session may take it over (logged). Every change is
# appended to ~/pixel7/log.txt.
set -u
DIR="$HOME/pixel7"
LOCK="$DIR/lock"          # a directory: mkdir is atomic
INFO="$LOCK/info"
LOG="$DIR/log.txt"
mkdir -p "$DIR"

now() { date -u +%s; }
stamp() { date -u +%Y-%m-%dT%H:%M:%SZ; }
log() { echo "$(stamp) $*" >> "$LOG"; }
hm() { date -u -r "$1" +%H:%MZ 2>/dev/null || date -u -d "@$1" +%H:%MZ; } # macOS, Linux
field() { sed -n "s/^$1=//p" "$INFO" 2>/dev/null; }

show() {
    if [ -d "$LOCK" ]; then
        local until; until=$(field until)
        echo "BUSY owner=$(field owner) what=$(field what) since=$(field since_utc) until=$(hm "${until:-0}")"
        return 1
    fi
    echo "FREE"
    return 0
}

write_info() { # owner minutes what
    { echo "owner=$1"; echo "what=$3"; echo "since_utc=$(stamp)"; echo "until=$(( $(now) + $2 * 60 ))"; } > "$INFO"
}

stale() {
    [ -d "$LOCK" ] || return 1
    local until; until=$(field until)
    [ -n "$until" ] && [ "$(now)" -gt $(( until + 600 )) ]
}

take() { # owner minutes what
    if stale; then
        log "STALE lock of $(field owner) ($(field what)) taken over by $1"
        rm -rf "$LOCK"
    fi
    if mkdir "$LOCK" 2>/dev/null; then
        write_info "$1" "$2" "$3"
        log "TAKE $1 for $2 min: $3"
        echo "TAKEN by $1 for $2 min"
        return 0
    fi
    if [ "$(field owner)" = "$1" ]; then
        write_info "$1" "$2" "$3"
        log "RETAKE $1 for $2 min: $3"
        echo "TAKEN by $1 for $2 min (already yours)"
        return 0
    fi
    show
    return 1
}

case "${1:-status}" in
    status) show ;;
    take) take "${2:?owner}" "${3:-30}" "${4:-test}" ;;
    wait)
        while ! take "${2:?owner}" "${3:-30}" "${4:-test}" >/dev/null; do sleep 60; done
        echo "TAKEN by $2 for ${3:-30} min" ;;
    extend)
        [ "$(field owner)" = "${2:?owner}" ] || { show; exit 1; }
        write_info "$2" "${3:-30}" "$(field what)"
        log "EXTEND $2 by ${3:-30} min"; echo "EXTENDED" ;;
    give)
        [ "$(field owner)" = "${2:?owner}" ] || { show; exit 1; }
        rm -rf "$LOCK"; log "GIVE $2"; echo "FREE" ;;
    *) sed -n '2,16p' "$0"; exit 2 ;;
esac
