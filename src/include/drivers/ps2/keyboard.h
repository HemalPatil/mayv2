#pragma once

#define IRQ_KEYBOARD 1

// keyboard.c
extern bool initializePs2Keyboard();
extern void ps2KeyboardHandler();

// keyboardasm.asm
extern void ps2KeyboardHandlerWrapper();
