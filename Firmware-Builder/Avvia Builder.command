#!/bin/zsh
cd -- "$(dirname -- "$0")" || exit 1
if command -v node >/dev/null 2>&1; then
  exec node server.mjs
elif [[ -x /opt/homebrew/bin/node ]]; then
  exec /opt/homebrew/bin/node server.mjs
elif [[ -x /usr/local/bin/node ]]; then
  exec /usr/local/bin/node server.mjs
else
  print 'Installa Node.js 18 o successivo, poi riapri questo file.'
  read -k 1
fi
