#include "input_handler.h"
#include "text_editor.h"
#include "file_manager.h"
#include "ble_keyboard.h"
#include "wifi_sync.h"

#include <Arduino.h>
#include <SDCardManager.h>

// External variables
extern bool autoReconnectEnabled;
extern bool darkMode;
extern bool cleanMode;
extern bool deleteConfirmPending;
extern bool switchAppConfirmPending;
extern WritingMode writingMode;
extern FontSize fontSize;
extern bool showWordCount;
extern KeyboardLayout keyboardLayout;

// External functions
void storePairedDevice(const std::string& address, const std::string& name);
bool getStoredDevice(std::string& address, std::string& name);
void clearStoredDevice();
uint32_t getCurrentPasskey();
bool isDeviceScanning();
void refreshScanNow();
void clearAllBluetoothBonds();

// --- Input Queue ---
static KeyEvent inputQueue[INPUT_QUEUE_SIZE];
static int queueHead = 0;
static int queueTail = 0;
static volatile bool queueFull = false;

// --- CapsLock state ---
static bool capsLockOn = false;

// --- Shared UTF-8 output buffer (used by hidToAsciiUTF8 and legacy hidToAscii) ---
extern char utf8buf[4];

// Where to return after title edit is confirmed or cancelled
static UIState renameReturnState = UIState::FILE_BROWSER;

// Forward declaration
static void openTitleEdit(const char* currentTitle, UIState returnTo);

// OTA app detection (defined in main.cpp)
extern OtaAppEntry otaApps[];
extern int otaAppCount;
void switchToOtaApp(int index);

// --- Shared UI state (defined in main.cpp) ---
extern UIState currentState;
extern int mainMenuSelection;
extern int selectedFileIndex;
extern int settingsSelection;
extern int bluetoothDeviceSelection;
extern int pairedKeyboardSelection;
extern Orientation currentOrientation;
extern int charsPerLine;
extern bool screenDirty;
extern char renameBuffer[];
extern int renameBufferLen;

void inputSetup() {
  queueHead = 0;
  queueTail = 0;
  queueFull = false;
  capsLockOn = false;
}

static bool isQueueEmpty() {
  return (queueHead == queueTail) && !queueFull;
}

void enqueueKeyEvent(uint8_t keyCode, uint8_t modifiers, bool pressed) {
  noInterrupts();
  if (!queueFull) {
    inputQueue[queueHead].keyCode = keyCode;
    inputQueue[queueHead].modifiers = modifiers;
    inputQueue[queueHead].pressed = pressed;
    queueHead = (queueHead + 1) % INPUT_QUEUE_SIZE;
    if (queueHead == queueTail) queueFull = true;
  }
  interrupts();
}

static KeyEvent dequeueKeyEvent() {
  KeyEvent event = {0, 0, false};
  noInterrupts();
  if (!isQueueEmpty()) {
    event = inputQueue[queueTail];
    queueTail = (queueTail + 1) % INPUT_QUEUE_SIZE;
    queueFull = false;
  }
  interrupts();
  return event;
}

// UTF-8 output buffer (shared between hidToAsciiUTF8 and callers)
char utf8buf[4];

int hidToAsciiUTF8(uint8_t hid, uint8_t modifiers) {
  bool shifted = isShift(modifiers) ^ capsLockOn;
  bool altGr   = isAltGr(modifiers);
  bool qwertz  = (keyboardLayout == KeyboardLayout::QWERTZ);

  // --- Letter keys ---
  if (hid >= 0x04 && hid <= 0x1D) {
    // AltGr specials must be handled here (letter path returns first)
    if (qwertz && altGr) {
      if (hid == 0x14) { utf8buf[0] = '@'; return 1; }  // @ on Q
      if (hid == 0x08) { utf8buf[0]='\342'; utf8buf[1]='\202'; utf8buf[2]='\254'; return 3; }  // € on E
      if (hid == 0x10) { utf8buf[0]='\302'; utf8buf[1]='\265'; return 2; }  // µ on M
    }
    if (qwertz) {
      if (hid == 0x1C) { utf8buf[0] = shifted ? 'Z' : 'z'; return 1; }
      if (hid == 0x1D) { utf8buf[0] = shifted ? 'Y' : 'y'; return 1; }
    }
    char base = 'a' + (hid - 0x04);
    utf8buf[0] = shifted ? (base - 32) : base;
    return 1;
  }

  // --- Number row (HID 0x1E-0x27) ---
  if (hid >= 0x1E && hid <= 0x27) {
    if (qwertz && altGr) {
      switch (hid) {
        case 0x1F: utf8buf[0]='\302'; utf8buf[1]='\262'; return 2; // ²
        case 0x20: utf8buf[0]='\302'; utf8buf[1]='\263'; return 2; // ³
        case 0x21: utf8buf[0] = '@';                     return 1;
        case 0x23: utf8buf[0] = '"';                     return 1;
        case 0x24: utf8buf[0] = '{';                     return 1;
        case 0x25: utf8buf[0] = '[';                     return 1;
        case 0x26: utf8buf[0] = ']';                     return 1;
        case 0x27: utf8buf[0] = '}';                     return 1;
      }
      return 0;
    }
    if (qwertz) {
      // Explicit per-key mapping — de_sh[] string literal is invalid because §/$ are multi-byte UTF-8
      static const char* de_us  = "1234567890";
      static const char de_us0  = '1', de_us1  = '2', de_us2  = '3', de_us3  = '4';
      static const char de_us4  = '5', de_us5  = '6', de_us6  = '7', de_us7  = '8';
      static const char de_us8  = '9', de_us9  = '0';
      static const char de_sh0  = '!';  // 1
      static const char de_sh1  = '"'; // 2
      static const char de_sh2a = '\302', de_sh2b = '\247'; // §
      static const char de_sh3  = '$';  // 4
      static const char de_sh4  = '%';  // 5
      static const char de_sh5  = '&';  // 6
      static const char de_sh6  = '/';  // 7
      static const char de_sh7  = '(';  // 8
      static const char de_sh8  = ')';  // 9
      static const char de_sh9  = '=';  // 0
      if (shifted) {
        switch (hid - 0x1E) {
          case 0: utf8buf[0] = de_sh0;  return 1;
          case 1: utf8buf[0] = de_sh1;  return 1;
          case 2: utf8buf[0] = de_sh2a; utf8buf[1] = de_sh2b; return 2; // §
          case 3: utf8buf[0] = de_sh3;  return 1;
          case 4: utf8buf[0] = de_sh4;  return 1;
          case 5: utf8buf[0] = de_sh5;  return 1;
          case 6: utf8buf[0] = de_sh6;  return 1;
          case 7: utf8buf[0] = de_sh7;  return 1;
          case 8: utf8buf[0] = de_sh8;  return 1;
          case 9: utf8buf[0] = de_sh9;  return 1;
        }
      }
      utf8buf[0] = de_us0 + (hid - 0x1E); return 1;
    }
    static const char us_us[] = "1234567890";
    static const char us_sh[] = "!@#$%^&*()";
    utf8buf[0] = shifted ? us_sh[hid-0x1E] : us_us[hid-0x1E];
    return 1;
  }

  // --- AltGr special keys (non-letter) ---
  if (altGr) {
    switch (hid) {
      case 0x2D: utf8buf[0] = '\\';                     return 1;  // \ (on ß)
      case 0x30: utf8buf[0] = '~';                      return 1;  // ~ (on =)
      case 0x31: utf8buf[0] = '|';                     return 1;  // | (on #)
    }
    return 0;
  }

  // --- Special keys ---
  switch (hid) {
    case 0x28: utf8buf[0] = '\n'; return 1;
    case 0x2B: utf8buf[0] = '\t'; return 1;
    case 0x2C: utf8buf[0] = ' ';  return 1;

    // ß / ? (US = - / _)
    case 0x2D:
      if (qwertz) {
        if (shifted) { utf8buf[0] = '?'; return 1; }  // ?
        utf8buf[0]='\303'; utf8buf[1]='\237'; return 2;                    // ß
      }
      utf8buf[0] = shifted ? '_' : '-'; return 1;

    // ^ / ° (US = = / +)
    case 0x2E:
      if (qwertz) {
        if (shifted) { utf8buf[0]='\302'; utf8buf[1]='\260'; return 2; }  // °
        utf8buf[0] = '^'; return 1;
      }
      utf8buf[0] = shifted ? '+' : '='; return 1;

    // Ü / ü (US = [ / {)
    case 0x2F:
      if (qwertz) {
        if (shifted) { utf8buf[0]='\303'; utf8buf[1]='\234'; return 2; }  // Ü
        utf8buf[0]='\303'; utf8buf[1]='\274'; return 2;                    // ü
      }
      utf8buf[0] = shifted ? '{' : '['; return 1;

    // + / * (US = ] / })
    case 0x30:
      if (qwertz) { utf8buf[0] = shifted ? '*' : '+'; return 1; }
      utf8buf[0] = shifted ? '}' : ']'; return 1;

    // # / $ (US = \ / |)
    case 0x31:
      if (qwertz) { utf8buf[0] = shifted ? '$' : '#'; return 1; }
      utf8buf[0] = shifted ? '|' : '\\'; return 1;

    // Ö / ö (US = ; / :)
    case 0x33:
      if (qwertz) {
        if (shifted) { utf8buf[0]='\303'; utf8buf[1]='\226'; return 2; }  // Ö
        utf8buf[0]='\303'; utf8buf[1]='\266'; return 2;                    // ö
      }
      utf8buf[0] = shifted ? ':' : ';'; return 1;

    // Ä / ä (US = ' / ")
    case 0x34:
      if (qwertz) {
        if (shifted) { utf8buf[0]='\303'; utf8buf[1]='\204'; return 2; }  // Ä
        utf8buf[0]='\303'; utf8buf[1]='\244'; return 2;                    // ä
      }
      utf8buf[0] = shifted ? '"' : '\''; return 1;

    // ` / ´ (US = ` / ~)
    case 0x35:
      if (qwertz) {
        if (shifted) { utf8buf[0]='\302'; utf8buf[1]='\264'; return 2; }  // ´
        utf8buf[0] = '\140';       return 1;                  // `
      }
      utf8buf[0] = shifted ? '~' : '`'; return 1;

    case 0x36: if (qwertz) { utf8buf[0] = shifted ? ';' : ','; return 1; } utf8buf[0] = shifted ? '<' : ','; return 1;
    case 0x37: if (qwertz) { utf8buf[0] = shifted ? ':' : '.'; return 1; } utf8buf[0] = shifted ? '>' : '.'; return 1;
    case 0x38: if (qwertz) { if (shifted) { utf8buf[0] = '>'; return 1; } if (altGr) { utf8buf[0] = '|'; return 1; } utf8buf[0] = '<'; return 1; } utf8buf[0] = shifted ? '|' : '\\'; return 1;

    default: return 0;
  }
}

// Legacy wrapper — returns first byte (for backwards compat with wifi_sync.cpp)
char hidToAscii(uint8_t hid, uint8_t modifiers) {
  hidToAsciiUTF8(hid, modifiers);
  return utf8buf[0];
}

// Handle text editor input
static void handleEditorKey(uint8_t keyCode, uint8_t modifiers) {
  // Ctrl shortcuts
  if (isCtrl(modifiers)) {
    if (keyCode == HID_KEY_S) {
      saveCurrentFile();
      screenDirty = true;
      return;
    }
    if (keyCode == HID_KEY_Z) {
      cleanMode = !cleanMode;
      screenDirty = true;
      return;
    }
    if (keyCode == HID_KEY_N) {
      openTitleEdit(editorGetCurrentTitle(), UIState::TEXT_EDITOR);
      return;
    }
    if (keyCode == HID_KEY_T) {
      writingMode = (writingMode == WritingMode::TYPEWRITER) ? WritingMode::NORMAL : WritingMode::TYPEWRITER;
      screenDirty = true;
      return;
    }
    if (keyCode == HID_KEY_F) {
      int v = static_cast<int>(fontSize);
      fontSize = static_cast<FontSize>((v + 1) % 3);
      screenDirty = true;
      return;
    }
    if (keyCode == HID_KEY_W) {
      showWordCount = !showWordCount;
      screenDirty = true;
      return;
    }
    if (keyCode == HID_KEY_P) {
      writingMode = (writingMode == WritingMode::PAGINATION) ? WritingMode::NORMAL : WritingMode::PAGINATION;
      screenDirty = true;
      return;
    }
    // Ctrl+Left/Right: jump pages in pagination mode
    if (writingMode == WritingMode::PAGINATION) {
      int pageSize = editorGetStoredVisibleLines();
      if (keyCode == HID_KEY_LEFT) {
        for (int i = 0; i < pageSize; i++) editorMoveCursorUp();
        screenDirty = true;
        return;
      }
      if (keyCode == HID_KEY_RIGHT) {
        for (int i = 0; i < pageSize; i++) editorMoveCursorDown();
        screenDirty = true;
        return;
      }
    }
    return;
  }

  // ESC = save and return to file browser
  if (keyCode == HID_KEY_ESCAPE) {
    if (editorHasUnsavedChanges()) saveCurrentFile();
    currentState = UIState::FILE_BROWSER;
    screenDirty = true;
    return;
  }

  // Tab cycles writing modes
  if (keyCode == HID_KEY_TAB) {
    int v = static_cast<int>(writingMode);
    writingMode = static_cast<WritingMode>((v + 1) % 3);
    screenDirty = true;
    return;
  }

  // Navigation keys
  switch (keyCode) {
    case HID_KEY_LEFT:      editorMoveCursorLeft();  screenDirty = true; return;
    case HID_KEY_RIGHT:     editorMoveCursorRight(); screenDirty = true; return;
    case HID_KEY_UP:        editorMoveCursorUp();    screenDirty = true; return;
    case HID_KEY_DOWN:      editorMoveCursorDown();  screenDirty = true; return;
    case HID_KEY_HOME:      editorMoveCursorHome();  screenDirty = true; return;
    case HID_KEY_END:       editorMoveCursorEnd();   screenDirty = true; return;
    case HID_KEY_BACKSPACE: editorDeleteChar();      screenDirty = true; return;
    case HID_KEY_DELETE:    editorDeleteForward();   screenDirty = true; return;
  }

  // CapsLock toggle
  if (keyCode == HID_KEY_CAPSLOCK) {
    capsLockOn = !capsLockOn;
    return;
  }

  // Printable character
  int n = hidToAsciiUTF8(keyCode, modifiers);
  for (int i = 0; i < n; i++) {
    editorInsertChar(utf8buf[i]);
  }
  if (n > 0) screenDirty = true;
}

// Open the title edit screen, returning to `returnTo` on confirm/cancel
static void openTitleEdit(const char* currentTitle, UIState returnTo) {
  strncpy(renameBuffer, currentTitle, MAX_TITLE_LEN - 1);
  renameBuffer[MAX_TITLE_LEN - 1] = '\0';
  renameBufferLen = strlen(renameBuffer);
  renameReturnState = returnTo;
  currentState = UIState::RENAME_FILE;
  screenDirty = true;
}

// Handle title edit input
static void handleRenameKey(uint8_t keyCode, uint8_t modifiers) {
  if (keyCode == HID_KEY_ENTER) {
    if (renameBufferLen > 0) {
      if (renameReturnState == UIState::TEXT_EDITOR) {
        editorSetCurrentTitle(renameBuffer);
        if (editorGetCurrentFile()[0] == '\0') {
          // New file — derive filename from title
          char filename[MAX_FILENAME_LEN];
          deriveUniqueFilename(renameBuffer, filename, MAX_FILENAME_LEN);
          editorSetCurrentFile(filename);
        } else {
          // Existing file — rename on disk to match new title
          updateFileTitle(editorGetCurrentFile(), renameBuffer);
        }
        editorSetUnsavedChanges(true);
        saveCurrentFile();
      } else {
        // Updating title of a file selected in the browser
        FileInfo* files = getFileList();
        updateFileTitle(files[selectedFileIndex].filename, renameBuffer);
      }
    }
    currentState = renameReturnState;
    screenDirty = true;
    return;
  }

  if (keyCode == HID_KEY_ESCAPE) {
    currentState = renameReturnState;
    screenDirty = true;
    return;
  }

  if (keyCode == HID_KEY_BACKSPACE) {
    if (renameBufferLen > 0) {
      renameBufferLen--;
      renameBuffer[renameBufferLen] = '\0';
      screenDirty = true;
    }
    return;
  }

  // Allow all printable characters in a title (including spaces)
  int n = hidToAsciiUTF8(keyCode, modifiers);
  for (int i = 0; i < n && renameBufferLen < MAX_TITLE_LEN - 1; i++) {
    renameBuffer[renameBufferLen++] = utf8buf[i];
  }
  renameBuffer[renameBufferLen] = '\0';
  if (n > 0) screenDirty = true;
}

static void dispatchEvent(const KeyEvent& event) {
  if (!event.pressed) return;

  switch (currentState) {
    case UIState::MAIN_MENU: {
      int menuCount = 4 + otaAppCount;
      if (switchAppConfirmPending) {
        if (event.keyCode == HID_KEY_ENTER) {
          switchAppConfirmPending = false;
          switchToOtaApp(mainMenuSelection - 4);
        } else {
          switchAppConfirmPending = false;
          screenDirty = true;
        }
        break;
      }
      if (event.keyCode == HID_KEY_DOWN) {
        mainMenuSelection = (mainMenuSelection + 1) % menuCount;
        screenDirty = true;
      } else if (event.keyCode == HID_KEY_UP) {
        mainMenuSelection = (mainMenuSelection - 1 + menuCount) % menuCount;
        screenDirty = true;
      } else if (event.keyCode == HID_KEY_ENTER) {
        if (mainMenuSelection == 0) {
          refreshFileList();
          currentState = UIState::FILE_BROWSER;
          screenDirty = true;
        } else if (mainMenuSelection == 1) {
          createNewFile();
          openTitleEdit("Untitled", UIState::TEXT_EDITOR);
        } else if (mainMenuSelection == 2) {
          currentState = UIState::SETTINGS;
          screenDirty = true;
        } else if (mainMenuSelection == 3) {
          wifiSyncStart();
          currentState = UIState::WIFI_SYNC;
          screenDirty = true;
        } else if (mainMenuSelection >= 4) {
          switchAppConfirmPending = true;
          screenDirty = true;
        }
      }
      break;
    }

    case UIState::FILE_BROWSER: {
      int fc = getFileCount();

      // Delete confirmation pending — Enter confirms, anything else cancels
      if (deleteConfirmPending) {
        if (event.keyCode == HID_KEY_ENTER && fc > 0) {
          FileInfo* files = getFileList();
          deleteFile(files[selectedFileIndex].filename);
          int newFc = getFileCount();
          if (selectedFileIndex >= newFc) selectedFileIndex = newFc - 1;
          if (selectedFileIndex < 0) selectedFileIndex = 0;
        }
        deleteConfirmPending = false;
        screenDirty = true;
        break;
      }

      if (event.keyCode == HID_KEY_DOWN && fc > 0) {
        selectedFileIndex = (selectedFileIndex + 1) % fc;
        screenDirty = true;
      } else if (event.keyCode == HID_KEY_UP && fc > 0) {
        selectedFileIndex = (selectedFileIndex - 1 + fc) % fc;
        screenDirty = true;
      } else if (event.keyCode == HID_KEY_ENTER && fc > 0) {
        FileInfo* files = getFileList();
        loadFile(files[selectedFileIndex].filename);
        screenDirty = true;
      } else if (isCtrl(event.modifiers) && event.keyCode == HID_KEY_N) {
        if (fc > 0) {
          FileInfo* files = getFileList();
          openTitleEdit(files[selectedFileIndex].title, UIState::FILE_BROWSER);
        }
      } else if (isCtrl(event.modifiers) && event.keyCode == HID_KEY_D) {
        if (fc > 0) {
          deleteConfirmPending = true;
          screenDirty = true;
        }
      } else if (event.keyCode == HID_KEY_ESCAPE) {
        currentState = UIState::MAIN_MENU;
        screenDirty = true;
      }
      break;
    }

    case UIState::TEXT_EDITOR:
      handleEditorKey(event.keyCode, event.modifiers);
      break;

    case UIState::RENAME_FILE:
      handleRenameKey(event.keyCode, event.modifiers);
      break;

    case UIState::SETTINGS: {
      const int SETTINGS_COUNT = 7;  // Orientation, Dark Mode, Writing Mode, Font Size, Keyboard Layout, Bluetooth, Paired Keyboards

      // Up/Down: navigate settings list (physical buttons also map here)
      if (event.keyCode == HID_KEY_DOWN) {
        settingsSelection = (settingsSelection + 1) % SETTINGS_COUNT;
        screenDirty = true;
      } else if (event.keyCode == HID_KEY_UP) {
        settingsSelection = (settingsSelection - 1 + SETTINGS_COUNT) % SETTINGS_COUNT;
        screenDirty = true;

      // Enter or Right: cycle setting forward
      } else if (event.keyCode == HID_KEY_ENTER || event.keyCode == HID_KEY_RIGHT) {
        if (settingsSelection == 0) {
          int v = static_cast<int>(currentOrientation);
          currentOrientation = static_cast<Orientation>((v + 1) % 4);
        } else if (settingsSelection == 1) {
          darkMode = !darkMode;
        } else if (settingsSelection == 2) {
          int v = static_cast<int>(writingMode);
          writingMode = static_cast<WritingMode>((v + 1) % 3);
        } else if (settingsSelection == 3) {
          int v = static_cast<int>(fontSize);
          fontSize = static_cast<FontSize>((v + 1) % 3);
        } else if (settingsSelection == 4) {
          int v = static_cast<int>(keyboardLayout);
          keyboardLayout = static_cast<KeyboardLayout>((v + 1) % 2);
        } else if (settingsSelection == 5) {
          currentState = UIState::BLUETOOTH_SETTINGS;
        } else if (settingsSelection == 6) {
          pairedKeyboardSelection = 0;
          currentState = UIState::PAIRED_KEYBOARDS;
        }
        screenDirty = true;

      // Left: cycle setting backward (keyboard only — physical L/R map to Up/Down)
      } else if (event.keyCode == HID_KEY_LEFT) {
        if (settingsSelection == 0) {
          int v = static_cast<int>(currentOrientation);
          currentOrientation = static_cast<Orientation>((v - 1 + 4) % 4);
        } else if (settingsSelection == 1) {
          darkMode = !darkMode;
        } else if (settingsSelection == 2) {
          int v = static_cast<int>(writingMode);
          writingMode = static_cast<WritingMode>((v - 1 + 3) % 3);
        } else if (settingsSelection == 3) {
          int v = static_cast<int>(fontSize);
          fontSize = static_cast<FontSize>((v - 1 + 3) % 3);
        } else if (settingsSelection == 4) {
          int v = static_cast<int>(keyboardLayout);
          keyboardLayout = static_cast<KeyboardLayout>((v + 1) % 2);
        }
        screenDirty = true;

      } else if (event.keyCode == HID_KEY_ESCAPE) {
        currentState = UIState::MAIN_MENU;
        screenDirty = true;
      }
      break;
    }

    case UIState::BLUETOOTH_SETTINGS: {
      int deviceCount = getDiscoveredDeviceCount();

      // Ensure selection is within bounds
      if (bluetoothDeviceSelection >= deviceCount && deviceCount > 0) {
        bluetoothDeviceSelection = deviceCount - 1;
      } else if (deviceCount == 0) {
        bluetoothDeviceSelection = 0; // Reset to 0 when no devices
      }

      if (event.keyCode == HID_KEY_ESCAPE) {
        DBG_PRINTLN("[INPUT] BT: Escape pressed - returning to settings");
        currentState = UIState::SETTINGS;
        screenDirty = true;
      } else if (event.keyCode == HID_KEY_DOWN) {
        if (deviceCount > 0) {
          bluetoothDeviceSelection = (bluetoothDeviceSelection + 1) % deviceCount;
          DBG_PRINTF("[INPUT] BT: Down pressed - selection now %d/%d\n", bluetoothDeviceSelection, deviceCount);
          screenDirty = true;
        }
      } else if (event.keyCode == HID_KEY_UP) {
        if (deviceCount > 0) {
          bluetoothDeviceSelection = (bluetoothDeviceSelection - 1 + deviceCount) % deviceCount;
          DBG_PRINTF("[INPUT] BT: Up pressed - selection now %d/%d\n", bluetoothDeviceSelection, deviceCount);
          screenDirty = true;
        }
      } else if (event.keyCode == HID_KEY_ENTER) {
        if (deviceCount > 0 && !isDeviceScanning()) {
          // Connect to the selected device
          connectToDevice(bluetoothDeviceSelection);
        } else if (!isDeviceScanning()) {
          // No devices — start a new scan
          startDeviceScan();
        }
        screenDirty = true;
      } else if (event.keyCode == HID_KEY_RIGHT) {
        // Right button = re-scan for devices
        if (!isDeviceScanning()) {
          startDeviceScan();
        }
        screenDirty = true;
      } else if (event.keyCode == HID_KEY_LEFT) {
        if (isKeyboardConnected()) {
          disconnectCurrentDevice();
          screenDirty = true;
        }
      }
      break;
    }

    case UIState::PAIRED_KEYBOARDS: {
      int count = getPairedKeyboardCount();

      if (event.keyCode == HID_KEY_ESCAPE) {
        currentState = UIState::SETTINGS;
        screenDirty = true;
      } else if (event.keyCode == HID_KEY_DOWN) {
        if (count > 0) {
          pairedKeyboardSelection = (pairedKeyboardSelection + 1) % count;
          screenDirty = true;
        }
      } else if (event.keyCode == HID_KEY_UP) {
        if (count > 0) {
          pairedKeyboardSelection = (pairedKeyboardSelection - 1 + count) % count;
          screenDirty = true;
        }
      } else if (event.keyCode == HID_KEY_ENTER) {
        if (count > 0) {
          connectToPairedKeyboard(pairedKeyboardSelection);
          currentState = UIState::SETTINGS;
          screenDirty = true;
        }
      } else if (event.keyCode == HID_KEY_D) {
        if (count > 0) {
          removePairedKeyboard(pairedKeyboardSelection);
          int newCount = getPairedKeyboardCount();
          if (pairedKeyboardSelection >= newCount && newCount > 0)
            pairedKeyboardSelection = newCount - 1;
          screenDirty = true;
        }
      } else if (event.keyCode == HID_KEY_LEFT) {
        // Disconnect if this keyboard is the currently connected one
        if (count > 0 && isKeyboardConnected()) {
          std::string addr, name; uint8_t addrType;
          getPairedKeyboard(pairedKeyboardSelection, addr, name, addrType);
          if (getCurrentDeviceAddress() == addr) {
            disconnectCurrentDevice();
            screenDirty = true;
          }
        }
      }
      break;
    }

    case UIState::WIFI_SYNC: {
      syncHandleKey(event.keyCode, event.modifiers);
      break;
    }

    default:
      break;
  }
}

int processAllInput() {
  int processedCount = 0;
  while (!isQueueEmpty()) {
    KeyEvent event = dequeueKeyEvent();
    dispatchEvent(event);
    processedCount++;
  }
  return processedCount;
}
