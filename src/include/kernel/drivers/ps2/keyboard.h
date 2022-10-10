#pragma once

#define IRQ_KEYBOARD 1

// keyboard.cpp
extern bool initializePs2Keyboard();
extern "C" void ps2KeyboardHandler();

// keyboardasm.asm
extern "C" void ps2KeyboardHandlerWrapper();
