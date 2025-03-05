#include "PPU_Bus.h"

namespace NES::PPU {
	namespace {
		u16 chip_select(u16 address) { // Acts as the discrete logic chip 74LS139 -> the NES combines a relatively small number of pins to produce a chip select signal for each of the individual components.
			if ((address & 0x1000)) {
				if (address & 0x0F00) return 3;
				else return 2;
			} else {
				return address & 0x2000;
			}
		}

		u16 get_palette_ram_address(u16 address) {
			address &= 0x001F;
			if ((address & 0x03) == 0) {
				return address & 0x0F;
			} else {
				return address;
			}
		}
	} // anonymous namespace

	// Writes Data to the Address Location on the Bus
	void PPU_Bus::write(u16 address, u8 data) {
		switch (chip_select(address)) {
		case 0: // $0000-1FFF -> Cartridge CHR-ROM/RAM -> Pattern Table
			assert(_card->ppu_read(address, _data)); // should never fail

		case 1:
			//_vRAM[]

		case 2:
			break;

		case 3: // Palette RAM
			_palette_RAM[get_palette_ram_address(address)] = data;
			break;

		default: break;
		}
	}

	// Reads Data from the Address Location on the Bus
	[[nodiscard]] u8 PPU_Bus::read(u16 address, bool bReadOnly) {
		switch (chip_select(address)) {
			case 0: // $0000-1FFF -> Cartridge CHR-ROM/RAM -> Pattern Table
			assert(_card->ppu_read(address, _data)); // should never fail
			return _data;

			case 1: // $2000-2FFF -> Nametable memory
			//_vRAM[]
			_data = _vRAM[0][0]; // TODO: implement this... 
			return _data;

			case 2: // $3000-3EFF -> Unused Cartridge Space
			break;

			case 3: // $3F00-3FFF -> Palette RAM
				return _palette_RAM[get_palette_ram_address(address)];

			default: 
			break;
		}

		return 0;
	}
}