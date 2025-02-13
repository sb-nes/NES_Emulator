#include "Bus.h"

namespace NES::CPU {
	namespace {

		u8 chip_select(u16 address) { // Acts as the discrete logic chip 74LS139 -> the NES combines a relatively small number of pins to produce a chip select signal for each of the individual components.
			if (!(address & 0x8000)) {
				return (address & 0x6000) >> 13;
			} else {
				return 3;
			}
		}

		bool chip_select_4000(u16 address) {
			return address > 0x401F;
		}

	} // Anonymous Namespace

	// Writes to the Address Bus
	void Bus::write(u16 address, u8 data) {
		assert(address >= 0x0000 && address <= 0xFFFF);

		switch (chip_select(address)) {
		case 0: // $0000 SRAM/WRAM
			_ram->write(address, data);
			break;

		case 1: // $2000-$0x3FFF PPU
			_ppu->write(address, data);
			break; 

		case 2: // $4000 I/O Registers + Cartridge
			if (chip_select_4000(address)) { // $4020-5FFF Cartridge
				_cartridge->cpu_write(address, data);
			} else { // $4000-401F I/O Registers

			}
			break; 

		case 3: // $6000 Cartridge
			_cartridge->cpu_write(address, data);
			break;

		default:
			break;
		}
	}

	// Reads from the Address Bus
	u8 Bus::read(u16 address, bool bReadOnly) {
		//assert(address); Can't assert, I'm using the whole range...
		assert(address >= 0x0000 && address <= 0xFFFF);

		switch (chip_select(address)) {
		case 0: // $0000 SRAM/WRAM
			return _ram->read(address);

		case 1: // $2000-$0x3FFF PPU
			return _ppu->read(address);

		case 2: // $4000 I/O Registers + Cartridge
			if (chip_select_4000(address)) // $4020-5FFF Cartridge
				return _cartridge->cpu_read(address);
			else { // $4000-401F I/O Registers

			}
			break;

		case 3: // $6000 Cartridge
#if !(CPU_TEST | RAM_TEST)
			return _cartridge->cpu_read(address);
#endif

		default:
			return 0x00; // Address Out of Range | Like how would this even happen? address range for uint_16 -> [0x0000, 0xFFFF] ??
		}
	}
}