#include <kernel.h>
#include <terminal.h>
#include <vbe.h>

bool setupGraphicalVideoMode() {
	VBEModeInfo *modes = (VBEModeInfo*)(uint64_t)infoTable->vbeModesInfoLocation;
	for (size_t i = 0; i < infoTable->vbeModesInfoCount; ++i) {
		terminalPrintDecimal((modes[i].attributes & VBE_LINEAR_FRAME_BUFFER_AVAILABLE) ? 1 : 0);
		terminalPrintChar(' ');
		terminalPrintDecimal(modes[i].width);
		terminalPrintChar(' ');
		terminalPrintDecimal(modes[i].height);
		terminalPrintChar(' ');
		terminalPrintDecimal(modes[i].bitsPerPixel);
		terminalPrintChar('\n');
	}
	terminalPrintDecimal(infoTable->vbeModesInfoCount);
	terminalPrintChar('\n');
	return true;
}