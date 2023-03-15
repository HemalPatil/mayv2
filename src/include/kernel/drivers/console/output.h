#pragma once

namespace Drivers {
namespace Console {
	class Output {
		public:
			enum Mode {
				None,
				TextVga,		// Should be used when only BPU is operating in VGA 80x25 mode
				TextVgaSmp,		// Should be used when all CPUs are operational in VGA 80x25 mode
				TextGraphical,		// Should be used when only BPU is operating in graphical text mode
				TextGraphicalSmp,	// Should be used when all CPUs are operational in graphical text mode
				GuiSmp,			// Should be used when all CPUs are operational in GUI graphical mode
			};

		private:
			Mode mode;
			Output();
			static Output instance;

		public:
			Output(const Output&) = delete;
			Output(Output&&) = delete;
			Output& operator=(const Output&) = delete;
			Output& operator=(Output&&) = delete;

			void get();

			static Output& getInstance();
	};

	extern Output &kout;
}
}
