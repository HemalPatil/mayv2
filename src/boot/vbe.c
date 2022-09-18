#include <kernel.h>
#include <string.h>
#include <terminal.h>
#include <vbe.h>

static const char* const switchingModeStr = "Switching to graphical video mode ";

bool setupGraphicalVideoMode() {
	// TODO: Go through all video modes and select the mode which has linear frame buffer
	// with the maximum width and height and bitsPerPixel preferrably 32-bit
	// For now select 1024x768x32 mode
	uint16_t *modeNumbers = (uint16_t*)infoTable->vbeModeNumbersLocation;
	VBEModeInfo *modes = (VBEModeInfo*)(uint64_t)infoTable->vbeModesInfoLocation;
	size_t selectedMode = SIZE_MAX;
	for (size_t i = 0; i < infoTable->vbeModesInfoCount; ++i) {
		// if (
		// 	selectedMode == SIZE_MAX ||
		// 	(
		// 		modes[i].attributes & VBE_LINEAR_FRAME_BUFFER_AVAILABLE &&
		// 		modes[i].width > modes[selectedMode].width &&
		// 		modes[i].height > modes[selectedMode].height &&
		// 		modes[i].bitsPerPixel > modes[selectedMode].bitsPerPixel
		// 	)
		// ) {
		// 	selectedMode = i;
		// }
		if (
			modes[i].attributes & VBE_LINEAR_FRAME_BUFFER_AVAILABLE &&
			modes[i].width == 1024 &&
			modes[i].height == 768 &&
			modes[i].bitsPerPixel == 32
		) {
			selectedMode = i;
		}
	}
	terminalPrintString(switchingModeStr, strlen(switchingModeStr));
	terminalPrintHex(&modeNumbers[selectedMode], 2);
	terminalPrintChar(' ');
	terminalPrintDecimal(modes[selectedMode].width);
	terminalPrintChar('x');
	terminalPrintDecimal(modes[selectedMode].height);
	terminalPrintChar('x');
	terminalPrintDecimal(modes[selectedMode].bitsPerPixel);
	terminalPrintChar('\n');
	return true;
}