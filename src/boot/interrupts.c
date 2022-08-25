#include <interrupts.h>
#include <kernel.h>

bool setupHardwareInterrupts() {
	// TODO : add ioapic and hardware irq redirection
	if(!apicExists()) {
		return false;
	}
	return true;
}

bool apicExists() {
	// TODO: add implementation
	return true;
}