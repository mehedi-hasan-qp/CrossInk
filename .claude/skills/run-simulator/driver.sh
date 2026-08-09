#!/usr/bin/env bash
# Driver for the CrossInk/CrossPoint desktop simulator (macOS only — it
# drives a real SDL2 window via `osascript`/`screencapture`, not a
# headless framebuffer). See ../run-simulator/SKILL.md for usage.
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
cd "$REPO_ROOT"

BIN=".pio/build/simulator/program"
PROC_NAME="program"          # process name macOS/System Events sees
LOG="/tmp/crossink_sim.log"

usage() {
  cat <<'EOF'
Usage: driver.sh <command> [args]

  build                 pio run -e simulator
  seed                  Copy a Bangla test EPUB + TXT into ./fs_/books/
  launch                Launch the built binary in the background, raise its window
  bounds                Print "x,y,w,h" of the simulator window
  shot <path.png>       Screenshot the simulator window to <path.png>
  key <name>            Send one keypress. name = back|confirm|left|right|up|down|power
  stop                  Kill the simulator process
EOF
}

cmd_build() {
  pio run -e simulator
}

cmd_seed() {
  mkdir -p fs_/books
  if [ -f test/epubs/test_bangla.epub ]; then
    cp test/epubs/test_bangla.epub fs_/books/bangla_test.epub
  else
    echo "warning: test/epubs/test_bangla.epub not found; run scripts/generate_bangla_test_epub.py first" >&2
  fi
  cat > fs_/books/bangla_test.txt <<'EOF'
বাংলা ভাষা পরীক্ষা

স্বরবর্ণ: অ আ ই ঈ উ ঊ ঋ এ ঐ ও ঔ
ব্যঞ্জনবর্ণ: ক খ গ ঘ ঙ চ ছ জ ঝ ঞ ট ঠ ড ঢ ণ ত থ দ ধ ন প ফ ব ভ ম য র ল শ ষ স হ

সংযুক্ত বর্ণ (): ক্ষ জ্ঞ ন্ত স্প ল্ল ক্র গ্র ন্য প্র ষ্ট

মাত্রা পরীক্ষা (ি-):
কি বি গি নি পি মি রি শি সি হি
কে বে গে নে পে মে রে শে সে হে

বাংলা সংখ্যা: ০ ১ ২ ৩ ৪ ৫ ৬ ৭ ৮ ৯

বাক্য পরীক্ষা:
আমি বাংলায় পড়তে পারি।
বিজ্ঞান ও প্রযুক্তি।
সংস্কৃতি ও সাহিত্য।
EOF
}

cmd_launch() {
  pkill -f "$BIN" 2>/dev/null || true
  sleep 0.3
  nohup "$BIN" > "$LOG" 2>&1 &
  sleep 2
  osascript -e "tell application \"System Events\" to tell process \"$PROC_NAME\" to perform action \"AXRaise\" of window 1" \
    || { echo "error: simulator window not found — check $LOG" >&2; exit 1; }
  wait_frontmost
}

# AXRaise can trigger a macOS Space-switch animation if the simulator
# window lives on a different virtual desktop than the active one.
# screencapture fired mid-transition can grab Mission Control's view of
# an unrelated desktop instead of the simulator. Never screenshot
# without confirming "program" is actually frontmost first.
frontmost_proc() {
  osascript -e 'tell application "System Events" to name of first process whose frontmost is true' 2>/dev/null
}

wait_frontmost() {
  local tries=0
  while [ "$(frontmost_proc)" != "$PROC_NAME" ]; do
    tries=$((tries + 1))
    if [ "$tries" -ge 20 ]; then
      echo "error: \"$PROC_NAME\" never became frontmost (still: $(frontmost_proc)) — Space switch may be stuck; bring the simulator window to your active desktop manually" >&2
      return 1
    fi
    sleep 0.3
  done
  # Extra settle time past the frontmost flag flip — the Space-switch
  # animation itself can still be finishing for a few frames after.
  sleep 0.5
}

cmd_bounds() {
  osascript <<EOF
tell application "System Events"
  tell process "$PROC_NAME"
    set {px, py} to position of window 1
    set {sw, sh} to size of window 1
    return (px as string) & "," & (py as string) & "," & (sw as string) & "," & (sh as string)
  end tell
end tell
EOF
}

cmd_shot() {
  local out="${1:?usage: driver.sh shot <path.png>}"
  wait_frontmost || exit 1
  local b; b="$(cmd_bounds)"
  screencapture -R"$b" -x "$out"
}

cmd_key() {
  local name="${1:?usage: driver.sh key <back|confirm|left|right|up|down|power>}"
  local code
  case "$name" in
    back)    code=53 ;;   # Escape -> BTN_BACK
    confirm) code=36 ;;   # Return -> BTN_CONFIRM
    left)    code=123 ;;  # Left arrow -> BTN_LEFT
    right)   code=124 ;;  # Right arrow -> BTN_RIGHT
    up)      code=126 ;;  # Up arrow -> BTN_UP
    down)    code=125 ;;  # Down arrow -> BTN_DOWN
    power)   code=35 ;;   # "P" -> BTN_POWER
    *) echo "unknown key: $name" >&2; exit 1 ;;
  esac
  wait_frontmost || exit 1
  osascript -e "tell application \"System Events\" to tell process \"$PROC_NAME\" to key code $code"
  sleep 0.5
}

cmd_stop() {
  pkill -f "$BIN" 2>/dev/null || true
}

case "${1:-}" in
  build) cmd_build ;;
  seed) cmd_seed ;;
  launch) cmd_launch ;;
  bounds) cmd_bounds ;;
  shot) cmd_shot "${2:-}" ;;
  key) cmd_key "${2:-}" ;;
  stop) cmd_stop ;;
  *) usage; exit 1 ;;
esac
