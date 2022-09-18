#pragma once

#include <stdbool.h>
#include <stdint.h>

#define VBE_LINEAR_FRAME_BUFFER_AVAILABLE 0x80

struct VBEModeInfo {
	uint16_t attributes;		// deprecated, only bit 7 should be of interest to you, and it indicates the mode supports a linear frame buffer.
	uint8_t windowA;			// deprecated
	uint8_t windowB;			// deprecated
	uint16_t granularity;		// deprecated; used while calculating bank numbers
	uint16_t windowSize;
	uint16_t segmentA;
	uint16_t segmentB;
	uint32_t windowFuncPtr;		// deprecated; used to switch banks from protected mode without returning to real mode
	uint16_t pitch;			// number of bytes per horizontal line
	uint16_t width;			// width in pixels
	uint16_t height;			// height in pixels
	uint8_t wChar;			// unused...
	uint8_t yChar;			// ...
	uint8_t planes;
	uint8_t bitsPerPixel;
	uint8_t banks;			// deprecated; total number of banks in this mode
	uint8_t memoryModel;
	uint8_t bankSize;		// deprecated; size of a bank, almost always 64 KB but may be 16 KB...
	uint8_t imagePages;
	uint8_t reserved0;
 
	uint8_t redMask;
	uint8_t redPosition;
	uint8_t greenMask;
	uint8_t greenPosition;
	uint8_t blueMask;
	uint8_t bluePosition;
	uint8_t reservedMask;
	uint8_t reservedPosition;
	uint8_t directColourAttributes;
 
	uint32_t frameBuffer;		// physical address of the linear frame buffer; write here to draw to the screen
	uint32_t offScreenMemOffset;
	uint16_t offScreenMemSize;	// size of memory in the framebuffer but not being displayed on the screen
	uint8_t reserved1[206];
} __attribute__ ((packed));
typedef struct VBEModeInfo VBEModeInfo;

extern bool setupGraphicalVideoMode();
