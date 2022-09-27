#pragma once

#define IRQ_KEYBOARD 1

// keyboard.c
extern bool initializePs2Keyboard();
extern void keyboardHandler();

// keyboardasm.asm
extern void keyboardHandlerWrapper();
