#include <drivers/video/vga.h>
#include <kernel.h>

#define VGA_LINEAR_FRAME_BUFFER_AVAILABLE 0x80

size_t Drivers::Video::VGA::currentMode = SIZE_MAX;

void Drivers::Video::VGA::selectMode() {
	uint16_t *modeNumbers = (uint16_t*)Kernel::infoTable.vbeModeNumbersLocation;
	ModeInfo *modes = (ModeInfo*)(uint64_t)Kernel::infoTable.vbeModesInfoLocation;
	for (size_t i = 0; i < Kernel::infoTable.vbeModesInfoCount; ++i) {

	}
}
