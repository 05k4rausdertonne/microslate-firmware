# Dual-boot MicroSlate + CrossInk on a USB-locked Xteink X4

This builds on the SD-card install method in [SD-CARD-INSTALL.md](SD-CARD-INSTALL.md).
It installs **MicroSlate** and **CrossInk** into the two OTA slots and lets either
app reboot into the other from its menu.

Both apps use the identical partition table (`app0` @ 0x10000, `app1` @ 0x650000,
6.25 MB each), and both self-mark-valid at boot, so whichever slot is booted
persists across reboots and deep-sleep.

## What you need (already built, on the `feat/dual-boot-switch` branches)

- **MicroSlate** — `lvturner/microslate-firmware` branch `feat/dual-boot-switch`.
  Adds: `esp_ota_mark_app_valid_cancel_rollback()` at end of `setup()`, and a
  raw-otadata slot switch (`otaBootSwitchTo`) that bypasses `esp_image_verify`
  (which rejects valid images with bogus eFuse-rev errors on these bootloaders).
- **CrossInk** — `lvturner/CrossInk` branch `feat/dual-boot-switch`.
  Adds: `mark_valid` at end of `setup()`, and a **Settings → System → "Switch to
  MicroSlate"** action (reuses CrossInk's `ota_boot::switchTo`).

Each project's `pio run` / `pio run -e default` output (`firmware.bin`) is already
a pure OTA app image — copy it to the SD card root **renamed `update.bin`**.

## Install (two SD flashes)

The OEM SD-card recovery always writes the *inactive* slot and switches to it, so
flashing CrossInk first then MicroSlate leaves **MicroSlate active** with CrossInk
in the other slot — regardless of starting state.

1. **Flash CrossInk** — copy CrossInk `firmware.bin` to the SD card as `update.bin`,
   plug in, hold **power + up**. Let CrossInk reach its home screen (so `mark_valid`
   runs and locks the slot).
2. **Flash MicroSlate** — copy MicroSlate `firmware.bin` to the SD card as
   `update.bin`, hold **power + up**. MicroSlate boots into the other slot
   (overwriting whatever was there) and becomes active.

End state: slot with MicroSlate is active; CrossInk is in the other slot.

## Switching

- **MicroSlate → CrossInk:** on MicroSlate's main menu, the entry below *Sync*
  (labelled `OTA Slot 0`/`OTA Slot 1`) is CrossInk — select it. MicroSlate writes
  otadata directly and reboots into CrossInk.
- **CrossInk → MicroSlate:** Settings → System → **Switch to MicroSlate** → confirm.

Both directions write otadata as a "pending" entry; the booted app confirms itself
valid on the next boot, so the switch sticks across further reboots.

## Notes

- MicroSlate labels the other app generically (`OTA Slot N`) because CrossInk
  doesn't register a friendly name in MicroSlate's NVS — that's cosmetic.
- If a switch ever appears to do nothing, re-flash the intended-primary app via SD
  (that rewrites the inactive slot and becomes active).
