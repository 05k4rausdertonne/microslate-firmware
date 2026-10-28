# German QWERTZ Keyboard Layout — HID Scancode Reference

## Key Principle

All keys use **US QWERTY HID codes** (the physical key position). The mapping changes based on:
- **Layout**: QWERTZ (German) vs QWERTY (US)
- **Shift**: Shift key or Caps Lock active
- **AltGr**: Right Alt key (third level)

QWERTZ differences from QWERTY:
- Y and Z keys are swapped (0x1D=Y, 0x1C=Z)
- Umlauts and ß occupy different key positions
- Different special characters on shifted/AltGr rows
- @ and € are AltGr+ keys, not Shift

---

## Letter Keys (HID 0x04–0x1D)

| HID | Key (US pos) | QWERTZ Unshifted | QWERTY Unshifted | QWERTZ Shifted | QWERTY Shifted | AltGr (DE) |
|-----|-------------|------------------|------------------|----------------|-----------------|------------|
| 0x04 | A | a | a | A | A | — |
| 0x05 | B | b | b | B | B | — |
| 0x06 | C | c | c | C | C | — |
| 0x07 | D | d | d | D | D | — |
| 0x08 | E | e | e | E | E | **€** (€) |
| 0x09 | F | f | f | F | F | — |
| 0x0A | G | g | g | G | G | — |
| 0x0B | H | h | h | H | H | — |
| 0x0C | I | i | i | I | I | — |
| 0x0D | J | j | j | J | J | — |
| 0x0E | K | k | k | K | K | — |
| 0x0F | L | l | l | L | L | — |
| 0x10 | M | m | m | M | M | **µ** |
| 0x11 | N | n | n | N | N | — |
| 0x12 | O | o | o | O | O | — |
| 0x13 | P | p | p | P | P | — |
| 0x14 | Q | q | q | Q | Q | **@** |
| 0x15 | R | r | r | R | R | — |
| 0x16 | S | s | s | S | S | — |
| 0x17 | T | t | t | T | T | — |
| 0x18 | U | u | u | U | U | — |
| 0x19 | V | v | v | V | V | — |
| 0x1A | W | w | w | W | W | — |
| 0x1B | X | x | x | X | X | — |
| 0x1C | Z (DE: Z) | z | y | Z | Y | — |
| 0x1D | Y (DE: Y) | y | z | Y | Z | — |

---

## Number Row (HID 0x1E–0x27)

| HID | Key | QWERTZ Unshifted | QWERTY Unshifted | QWERTZ Shifted | QWERTY Shifted | AltGr (DE) |
|-----|-----|------------------|------------------|----------------|-----------------|------------|
| 0x1E | 1 | 1 | 1 | ! | ! | — |
| 0x1F | 2 | 2 | 2 | " | @ | **²** |
| 0x20 | 3 | 3 | 3 | **§** | # | **³** |
| 0x21 | 4 | 4 | 4 | **$** | $ | **@** |
| 0x22 | 5 | 5 | 5 | % | % | — |
| 0x23 | 6 | 6 | 6 | & | ^ | **"** |
| 0x24 | 7 | 7 | 7 | / | ' | **{** |
| 0x25 | 8 | 8 | 8 | ( | [ | **[*]** |
| 0x26 | 9 | 9 | 9 | ) | ] | **]** |
| 0x27 | 0 | 0 | 0 | = | } | **}** |

**Notes:**
- US 0x21 (4) shifts to `$` on both layouts, but on DE AltGr it maps to `@`
- US 0x23 (6) shifts to `^` on US, `&` on DE
- US 0x28 (7) shifts to `'` on US, `/` on DE
- US 0x2A (8) shifts to `[` on US, `(` on DE
- US 0x2B (9) shifts to `]` on US, `)` on DE
- US 0x2C (10) shifts to `}` on US, `=` on DE

---

## Special Keys (HID 0x2D–0x38)

| HID | Key (US pos) | QWERTZ Unshifted | QWERTY Unshifted | QWERTZ Shifted | QWERTY Shifted | AltGr (DE) |
|-----|-------------|------------------|------------------|----------------|-----------------|------------|
| 0x2D | ß (DE: ß) | **ß** | - | **§** | _ | **\** |
| 0x2E | ´ (DE: ^) | **^** | = | **°** | + | — |
| 0x2F | ü (DE: ü) | **ü** | [ | **Ü** | { | — |
| 0x30 | + (DE: =) | + | ] | = | } | **~** |
| 0x31 | # (DE: #) | # | \ | **$** | \| | **\|** |
| 0x33 | ö (DE: ö) | **ö** | ; | **Ö** | : | — |
| 0x34 | ä (DE: ä) | **ä** | ' | **Ä** | " | — |
| 0x35 | ` (DE: `) | ` | ` | **´** | ~ | — |
| 0x36 | , | , | , | < | < | — |
| 0x37 | . | . | . | > | > | — |
| 0x38 | - | - | / | ? | / | — |

**Notes:**
- US 0x30 is `]`, DE 0x30 is `+` (unshifted) or `=` (shifted)
- US 0x31 is `\`, DE 0x31 is `#` (unshifted) or `$` (shifted), AltGr=`|`
- US 0x33 is `:`, DE 0x33 is `ö` (unshifted) or `Ö` (shifted)
- US 0x34 is `"`, DE 0x34 is `ä` (unshifted) or `Ä` (shifted)
- US 0x35 is `~`, DE 0x35 is `` ` `` (unshifted) or `´` (shifted)

---

## UTF-8 Encoding Reference

| Char | Unicode | UTF-8 Bytes (hex) | UTF-8 Bytes (octal) | Size |
|------|---------|-------------------|---------------------|------|
| € | U+20AC | E2 82 AC | \342 \202 \254 | 3 bytes |
| µ | U+00B5 | C2 B5 | \302 \265 | 2 bytes |
| § | U+00A7 | C2 A7 | \302 \247 | 2 bytes |
| ° | U+00B0 | C2 B0 | \302 \260 | 2 bytes |
| ² | U+00B2 | C2 B2 | \302 \262 | 2 bytes |
| ³ | U+00B3 | C2 B3 | \302 \263 | 2 bytes |
| µ (mu) | U+03BC | CE BC | \316 \274 | 2 bytes (alt form) |
| ß | U+00DF | C3 9F | \303 \237 | 2 bytes |
| Ä | U+00C4 | C3 84 | \303 \204 | 2 bytes |
| ä | U+00E4 | C3 A4 | \303 \244 | 2 bytes |
| Ö | U+00D6 | C3 96 | \303 \226 | 2 bytes |
| ö | U+00F6 | C3 B6 | \303 \266 | 2 bytes |
| Ü | U+00DC | C3 9C | \303 \234 | 2 bytes |
| ü | U+00FC | C3 BC | \303 \274 | 2 bytes |
| $ (DE shifted) | U+0024 | 24 | \044 | 1 byte (ASCII) |
| $ (DE AltGr) | U+20AC | E2 82 AC | \342 \202 \254 | 3 bytes (€) |
| $ (DE shifted 0x31) | U+0024 | 24 | \044 | 1 byte (ASCII) |

**IMPORTANT**: When assigning multi-byte UTF-8 characters in C, you **MUST** store each byte separately:

```c
// WRONG — crashes firmware: multi-char literal, implementation-defined value
utf8buf[0] = '\303\237'; return 1;  // Only stores one byte, corrupts UTF-8

// CORRECT — stores both bytes, returns byte count
utf8buf[0] = '\303'; utf8buf[1] = '\237'; return 2;  // ß
```

---

## Dead Keys

The following keys are **dead keys** — they produce a combining accent that is applied to the next letter pressed:

| HID | Key | Unshifted | Shifted | Notes |
|-----|-----|-----------|---------|-------|
| 0x2E | ^ | ^ (circumflex) | ° (degree) | As dead key: ^ + a = â |
| 0x35 | ` | ` (backtick) | ´ (acute) | As dead key: ` + e = è; ´ + a = á |

Dead key sequences produce composed Unicode characters when followed by a compatible letter.

---

## Special Characters on AltGr

| AltGr+Key | QWERTZ Output | Description |
|-----------|--------------|-------------|
| AltGr+E | € | Euro sign |
| AltGr+M | µ | Micro sign |
| AltGr+ß | \ | Backslash |
| AltGr+= | ~ | Tilde |
| AltGr+# | \| | Vertical bar |
| AltGr+2 | ² | Superscript 2 |
| AltGr+3 | ³ | Superscript 3 |
| AltGr+Q | @ | At sign |
| AltGr+6 | " | Double quote |
| AltGr+7 | { | Left brace |
| AltGr+8 | [ | Left bracket |
| AltGr+9 | ] | Right bracket |
| AltGr+0 | } | Right brace |
| AltGr+4 | @ | At sign |

---

## Comparison: QWERTZ vs QWERTY

| HID | Character | QWERTZ (Shifted) | QWERTY (Shifted) | Difference |
|-----|-----------|------------------|------------------|------------|
| 0x1C | Key 1 | Z | Y | Y/Z swapped |
| 0x1D | Key 2 | Y | Z | Y/Z swapped |
| 0x20 | Key 3 | § | # | § vs # |
| 0x21 | Key 4 | $ | $ | Same (unshifted) |
| 0x23 | Key 6 | & | ^ | & vs ^ |
| 0x24 | Key 7 | / | ' | / vs ' |
| 0x25 | Key 8 | ( | [ | ( vs [ |
| 0x26 | Key 9 | ) | ] | ) vs ] |
| 0x27 | Key 0 | = | } | = vs } |
| 0x2D | ß/- | ß/§ | -/_ | Full remap |
| 0x2E | ^/° | ^/° | =/+ | Remapped |
| 0x2F | ü/Ü | ü/Ü | [/{ | Full remap |
| 0x30 | +/= | +/= | ]/} | Remapped |
| 0x31 | #/$ | #/$ | \/\| | Remapped |
| 0x33 | ö/Ö | ö/Ö | ;/: | Full remap |
| 0x34 | ä/Ä | ä/Ä | '/" | Full remap |
| 0x35 | `/´ | `/´ | `/~ | Remapped |

Total differences: ~24 key positions produce different characters between layouts.
