#!/usr/bin/env bash
# Workaround for a PlatformIO + ESP-IDF build failure on systems where /home is
# a symlink (e.g. Fedora Silverblue/Kinoite, distrobox/toolbox).
#
# Symptom: build fails with
#   "Two environments with different actions were specified for the same target: ...mem.c.o"
#   "Multiple ways to build the same target were specified for: ...esp_efuse_fields.c.o"
#
# Cause: PlatformIO's espidf.py compares each source path against the ESP-IDF
# components dir as plain strings. When /home -> /var/home, the prefix check
# fails and every same-named source across ESP-IDF components collapses onto one
# flattened object name and collides.
#
# This patch normalizes both sides with os.path.realpath. Idempotent: safe to
# re-run, and should be re-run after any espressif32 platform re-install.
set -euo pipefail

F="${PLATFORMIO_HOME:-$HOME/.platformio}/platforms/espressif32/builder/frameworks/espidf.py"

if [ ! -f "$F" ]; then
  echo "espidf.py not found at: $F" >&2
  echo "Run 'pio run' once first so PlatformIO installs the espressif32 platform." >&2
  exit 1
fi

python3 - "$F" <<'PY'
import sys
p = sys.argv[1]
s = open(p, encoding="utf-8").read()
orig = s

a = 'components_dir = fs.to_unix_path(os.path.join(FRAMEWORK_DIR, "components"))'
b = 'components_dir = os.path.realpath(fs.to_unix_path(os.path.join(FRAMEWORK_DIR, "components")))'
if b in s:
    pass
elif a in s:
    s = s.replace(a, b)
else:
    print("Could not find the components_dir line (already patched or upstream changed).", file=sys.stderr)

c = "if src_path.lower().startswith(components_dir.lower()):"
d = "if os.path.realpath(src_path).lower().startswith(components_dir.lower()):"
if d in s:
    pass
elif c in s:
    s = s.replace(c, d)
else:
    print("Could not find the startswith comparison (already patched or upstream changed).", file=sys.stderr)

if s != orig:
    open(p, "w", encoding="utf-8").write(s)
    print("Patched:", p)
else:
    print("Already patched (no changes needed):", p)
PY
