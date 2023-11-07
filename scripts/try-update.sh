#!/usr/bin/env bash

OUT="$1"

if [ ! -f "$OUT" ]; then
    cat > "$OUT"
else
    f="$(mktemp)"
    trap 'rm "$f"' -0
    cat > "$f"
    if cmp -s -- "$f" "$OUT"; then
        # Up to date, ignore
        :
    else
        cp "$f" "$OUT"
    fi
fi
