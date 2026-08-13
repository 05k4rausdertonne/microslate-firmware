# Installing MicroSlate on a USB-locked Xteink X4 (SD-card method)

This fork adds one thing: **MicroSlate survives reboots and deep-sleep** when
installed via the SD-card OEM bootloader recovery on a USB-locked X4.

If your device boots MicroSlate once and then "drops back to stock firmware" on
the next reboot, sleep-wake, or after holding Back — you're in the right place.

---

## Why stock MicroSlate reverts

Some Xteink X4 units are **USB-locked**: the USB port is a fixed HID device
(`5548:6674`) with no CDC serial and no Espressif download mode. You cannot use
`esptool`, the Web Serial installer, or `pio run -t upload`. The only install
path is the device's **OEM bootloader SD-card recovery**:

1. Copy an `update.bin` (a pure OTA app image) to the SD card root.
2. Hold **power + up** while booting.
3. The OEM bootloader flashes `update.bin` into an OTA slot.

The catch: a slot written this way boots **once** in a *"pending verify"* state.
The stock bootloader then **rolls back to the previous (stock) slot on the next
reset** unless the freshly-booted app self-confirms by calling
`esp_ota_mark_app_valid_cancel_rollback()`.

Upstream MicroSlate never makes that call — so it boots once, then every reset
(reboot, deep-sleep wake, Back-hold restart) reverts to stock.

> The OTA / rollback mechanism was reverse-engineered and documented by the
> CrossPoint project — see
> [`crosspoint-reader/crosspoint-tools`](https://github.com/crosspoint-reader/crosspoint-tools),
> in particular its `INTEGRATION.md` § "Recovery considerations". This fork
> simply applies their documented fix.

## The fix in this fork

A single line at the end of `setup()` in `src/main.cpp`, after all hardware and
UI init **and** the first display refresh, so the slot is only marked valid once
MicroSlate has provably come up:

```cpp
esp_ota_mark_app_valid_cancel_rollback();
```

Once it boots successfully a single time, the bootloader will never roll that
slot back to stock.

---

## Build

Requirements: Python 3, PlatformIO (`pip install platformio`), ~4 GB free disk.
Linux x86_64 or Windows (the ESP-IDF toolchain does not support Mac ARM / Pi).

```bash
git clone https://github.com/lvturner/microslate-firmware
cd microslate-firmware
pio run
```

The build produces `.pio/build/xteink_x4/firmware.bin`, which is **already a
pure OTA app image** (starts with `0xE9`, no bootloader or partition table
inside). That is exactly the format the OEM SD-card recovery expects — no
extraction or conversion step is needed.

### Build gotcha — `/home` is a symlink (Fedora Silverblue / Kinoite / distrobox)

**Symptom:** the build fails with one of:

```
Two environments with different actions were specified for the same target: ...mem.c.o
Multiple ways to build the same target were specified for: ...esp_efuse_fields.c.o
```

**Cause:** PlatformIO's ESP-IDF integration compares source paths against the
ESP-IDF `components` directory as plain strings. When `/home` is a symlink (e.g.
`/home -> /var/home` on immutable Fedora), the prefix check fails and every
same-named source file across ESP-IDF components (`mem.c`, `esp_efuse_fields.c`,
…) collapses onto the same flattened object name and collides.

**Fix:** run the included helper — it `realpath`-normalizes both sides of the
comparison in PlatformIO's `espidf.py`. It is idempotent; re-run it after any
`platformio` platform re-install.

```bash
scripts/fix-platformio-home-symlink.sh
```

---

## Flash via SD card

1. Copy the build output to the SD card root **renamed as `update.bin`**:
   ```bash
   cp .pio/build/xteink_x4/firmware.bin /path/to/SD/update.bin
   ```
2. The SD card must be **FAT32**.
3. Insert the card, plug in USB (power or PC), and hold **power + up** until the
   device boots. The OEM bootloader flashes `update.bin`.
4. MicroSlate boots. The slot is marked valid at the end of `setup()`.
5. **Test the two things that used to fail:**
   - Power-cycle once → should stay in MicroSlate.
   - Leave it ~5 minutes to deep-sleep, then wake it → should wake into MicroSlate.

---

## Notes / caveats

- **No USB serial.** On USB-locked units the port is HID-only, so there is no
  serial console and `pio device monitor` will not work. The SD path is the only
  way on or off the device.
- **"OTA Slot 0" menu entry.** The MicroSlate main menu scans the other OTA
  slot and will still show the stock firmware as a 5th entry below *Sync*
  (labelled `OTA Slot 0`). Selecting it intentionally switches back to stock —
  it is not a bug. Don't navigate there unless you want stock.
- **Install replaces the inactive slot.** Flashing writes the inactive OTA slot
  and switches to it; the previous firmware (stock) remains in the other slot
  as the rollback target / "OTA Slot 0" entry.
