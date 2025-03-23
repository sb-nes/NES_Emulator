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
				_ram.write(address, data);
			break;

			case 1: // $2000-$0x3FFF PPU
				_ppu.cpubus_write(address, data);
			break; 

			case 2: // $4000 I/O Registers + Cartridge
				if (chip_select_4000(address)) { // $4020-5FFF Cartridge
					_cartridge->cpu_write(address, data);
				} else { // $4000-401F I/O Registers

					if (address == 0x4014) {
						_dma_page = data;
						_dma_address = 0x00;
						_dma_enabled = true;
					}
					else if (address == 0x4016 || address == 0x4017) { // TODO: Fix Controllers
						//_controller[0]->clock();
						//_controller[0]->write(address, data & 0x01);
						//_controller[1]->clock();
						//_controller[1]->write(address, data & 0x01); 
						controller_state[address & 0x01] = controller[address & 0x01];
					}
				}
			break; 

			case 3: // $6000 Cartridge
				_cartridge->cpu_write(address, data);
			break;

			default: 
				assert(false && "Failed to Write on CPU Bus!");
			break;
		}
	}

	// Reads from the Address Bus
	u8 Bus::read(u16 address, bool bReadOnly) {
		assert(address >= 0x0000 && address <= 0xFFFF);

		switch (chip_select(address)) {
			// $0000 SRAM/WRAM
			case 0: return _ram.read(address);

			case 1: // $2000-$0x3FFF PPU
				return _ppu.cpubus_read(address);

			case 2: // $4000 I/O Registers + Cartridge
				if (chip_select_4000(address)) // $4020-5FFF Cartridge
					return _cartridge->cpu_read(address);
				else { // $4000-401F I/O Registers

					if (address == 0x4016 || address == 0x4017) {
						//_controller[address & 0x01]->clock();
						//return _controller[address & 0x01]->read(address);

						u8 data = (controller_state[address & 0x0001] & 0x80) > 0;
						controller_state[address & 0x0001] <<= 1;
						return data;
					}
				}
			break;

			case 3: // $6000 Cartridge
#if !(CPU_TEST | RAM_TEST)
				return _cartridge->cpu_read(address);
#else
				switch (address) {

				// NMI Handler Address:
				case 0xFFFA: return 0x00;
				case 0xFFFB: return 0x00;

				// Program Address:
				case 0xFFFC: return 0x00;
				case 0xFFFD: return 0x00;

				// IRQ Handler Address:
				case 0xFFFE: return 0x00;
				case 0xFFFF: return 0x07;

		}
#endif

			default: break;
		}

		return 0x00; // Address Out of Range | Like how would this even happen? address range for uint_16 -> [0x0000, 0xFFFF] ??
	}
}