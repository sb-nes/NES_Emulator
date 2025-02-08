#pragma once

#include "../Common/CommonHeaders.h"

namespace NES::PPU { // Picture Processing Unit
	class R2C02 {
	public:
		R2C02() {

		}

		// CPU Address BUS read and write:

		// Writes to the Address Bus
		void cpubus_write(u16 address, u8 data);
		// Reads from the Address Bus
		u8 cpubus_read(u16 address, bool bReadOnly = false);

		// Writes to the PPU's Address Bus
		void write(u16 address, u8 data);
		// Reads from the PPU's Address Bus
		u8 read(u16 address, bool bReadOnly = false);

		void clock() {
			// Clock function of the PPU
		}

	private:
		//NES::CPU::Bus* _bus;

		s16 _scanline{ 0 };
		s16 _cycle{ 0 };

	};
}