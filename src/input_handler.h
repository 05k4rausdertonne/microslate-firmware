#pragma once

#include "config.h"

void inputSetup();
void enqueueKeyEvent(uint8_t keyCode, uint8_t modifiers, bool pressed);
int processAllInput();
int hidToAsciiUTF8(uint8_t hid, uint8_t modifiers);
char hidToAscii(uint8_t hid, uint8_t modifiers);
