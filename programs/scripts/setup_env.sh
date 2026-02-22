#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ADD_FILE="$SCRIPT_DIR/add_to_profile.txt"

cat "$ADD_FILE" >> "$HOME/.profile"
echo "" >> "$HOME/.profile"

echo "Appended OTTER environment to ~/.profile"
echo "Open a new terminal or run: source $ADD_FILE"