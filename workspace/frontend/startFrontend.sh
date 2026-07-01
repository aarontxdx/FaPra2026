#!/bin/bash
set -e

cd "$(dirname "$0")" || exit 1
URL="http://localhost:8000/WebView.html"

python3 -m http.server 8000 &
SERVER_PID=$!

sleep 1

if command -v xdg-open >/dev/null 2>&1; then
    xdg-open "$URL"
else
    echo "Open manually: $URL"
fi

wait "$SERVER_PID"