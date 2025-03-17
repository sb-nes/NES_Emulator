#include "PPU_Bus.h"

namespace NES::PPU {
	namespace {
		u16 chip_select(u16 address) { // Acts as the discrete logic chip 74LS139 -> the NES combines a relatively small number of pins to produce a chip select signal for each of the individual components.
			if ((address & 0x2000)) {
				if (!(address & 0x1000)) return 1;
				else if (address & 0x0F00)
					return 3;
				else
					return 2;
			} else {
				return 0;
			}
		}

		u16 get_palette_ram_address(u16 address) {
			address &= 0x001F;
			if ((address & 0x03) == 0) return address & 0x0F;
			else return address;
		}
	} // anonymous namespace

	// Writes Data to the Address Location on the Bus
	void PPU_Bus::write(u16 address, u8 data) {
		bool result{ false };
		switch (chip_select(address)) {
			case 0: // $0000-1FFF -> Cartridge CHR-ROM/RAM -> Pattern Table
				result = _card->ppu_read(address, _data);
				assert(result); // should never fail
			break;

			case 1:
				address &= 0x0FFF;
				if (_card->_mirror == Cartridge::GameCard::Mirror::VERTICAL) {
					if (address >= 0x0000 && address <= 0x03FF) _data = _vRAM[address & 0x03FF];
					if (address >= 0x0400 && address <= 0x07FF) _data = _vRAM[0x03FF + (address & 0x03FF)];
					if (address >= 0x0800 && address <= 0x0BFF) _data = _vRAM[address & 0x03FF];
					if (address >= 0x0C00 && address <= 0x0FFF) _data = _vRAM[0x03FF + (address & 0x03FF)];
				}
				else if (_card->_mirror == Cartridge::GameCard::Mirror::HORIZONTAL) {
					if (address >= 0x0000 && address <= 0x03FF) _data = _vRAM[address & 0x03FF];
					if (address >= 0x0400 && address <= 0x07FF) _data = _vRAM[address & 0x03FF];
					if (address >= 0x0800 && address <= 0x0BFF) _data = _vRAM[0x03FF + (address & 0x03FF)];
					if (address >= 0x0C00 && address <= 0x0FFF) _data = _vRAM[0x03FF + (address & 0x03FF)];
				}
			break;

			case 2: // $3000-3EFF -> Unused Cartridge Space
				// temp
				address &= 0x0FFF;
				if (_card->_mirror == Cartridge::GameCard::Mirror::VERTICAL) {
					if (address >= 0x0000 && address <= 0x03FF) _data = _vRAM[address & 0x03FF];
					if (address >= 0x0400 && address <= 0x07FF) _data = _vRAM[0x03FF + (address & 0x03FF)];
					if (address >= 0x0800 && address <= 0x0BFF) _data = _vRAM[address & 0x03FF];
					if (address >= 0x0C00 && address <= 0x0FFF) _data = _vRAM[0x03FF + (address & 0x03FF)];
				}
				else if (_card->_mirror == Cartridge::GameCard::Mirror::HORIZONTAL) {
					if (address >= 0x0000 && address <= 0x03FF) _data = _vRAM[address & 0x03FF];
					if (address >= 0x0400 && address <= 0x07FF) _data = _vRAM[address & 0x03FF];
					if (address >= 0x0800 && address <= 0x0BFF) _data = _vRAM[0x03FF + (address & 0x03FF)];
					if (address >= 0x0C00 && address <= 0x0FFF) _data = _vRAM[0x03FF + (address & 0x03FF)];
				}
			break;

			case 3: // $3F00-3FFF -> Palette RAM
				_palette_RAM[get_palette_ram_address(address)] = data;
			break;

		default: break;
		}
	}

	// Reads Data from the Address Location on the Bus
	[[nodiscard]] u8 PPU_Bus::read(u16 address, bool bReadOnly) {
		bool result{ false };
		switch (chip_select(address)) {
			case 0: // $0000-1FFF -> Cartridge CHR-ROM/RAM -> Pattern Table
				result = _card->ppu_read(address, _data);
				assert(result); // should never fail
				return _data;

			case 1: // $2000-2FFF -> Nametable memory
				address &= 0x0FFF;
				if (_card->_mirror == Cartridge::GameCard::Mirror::VERTICAL) {
					if (address >= 0x0000 && address <= 0x03FF) _data = _vRAM[address & 0x03FF];
					if (address >= 0x0400 && address <= 0x07FF) _data = _vRAM[0x03FF + (address & 0x03FF)];
					if (address >= 0x0800 && address <= 0x0BFF) _data = _vRAM[address & 0x03FF];
					if (address >= 0x0C00 && address <= 0x0FFF) _data = _vRAM[0x03FF + (address & 0x03FF)];
				}
				else if (_card->_mirror == Cartridge::GameCard::Mirror::HORIZONTAL) {
					if (address >= 0x0000 && address <= 0x03FF) _data = _vRAM[address & 0x03FF];
					if (address >= 0x0400 && address <= 0x07FF) _data = _vRAM[address & 0x03FF];
					if (address >= 0x0800 && address <= 0x0BFF) _data = _vRAM[0x03FF + (address & 0x03FF)];
					if (address >= 0x0C00 && address <= 0x0FFF) _data = _vRAM[0x03FF + (address & 0x03FF)];
				}
				/*
				if (_card->_mirror == Cartridge::GameCard::Mirror::VERTICAL) {
					if (address >= 0x0000 && address <= 0x03FF) _data = _vRAM[0][address & 0x03FF];
					if (address >= 0x0400 && address <= 0x07FF) _data = _vRAM[1][address & 0x03FF];
					if (address >= 0x0800 && address <= 0x0BFF) _data = _vRAM[0][address & 0x03FF];
					if (address >= 0x0C00 && address <= 0x0FFF) _data = _vRAM[1][address & 0x03FF];
				} else if (_card->_mirror == Cartridge::GameCard::Mirror::HORIZONTAL) {
					if (address >= 0x0000 && address <= 0x03FF) _data = _vRAM[0][address & 0x03FF];
					if (address >= 0x0400 && address <= 0x07FF) _data = _vRAM[0][address & 0x03FF];
					if (address >= 0x0800 && address <= 0x0BFF) _data = _vRAM[1][address & 0x03FF];
					if (address >= 0x0C00 && address <= 0x0FFF) _data = _vRAM[1][address & 0x03FF];
				}
				*/
				return _data;

			case 2: // $3000-3EFF -> Unused Cartridge Space
				address &= 0x0FFF;
				if (_card->_mirror == Cartridge::GameCard::Mirror::VERTICAL) {
					if (address >= 0x0000 && address <= 0x03FF) _data = _vRAM[address & 0x03FF];
					if (address >= 0x0400 && address <= 0x07FF) _data = _vRAM[0x03FF + (address & 0x03FF)];
					if (address >= 0x0800 && address <= 0x0BFF) _data = _vRAM[address & 0x03FF];
					if (address >= 0x0C00 && address <= 0x0FFF) _data = _vRAM[0x03FF + (address & 0x03FF)];
				}
				else if (_card->_mirror == Cartridge::GameCard::Mirror::HORIZONTAL) {
					if (address >= 0x0000 && address <= 0x03FF) _data = _vRAM[address & 0x03FF];
					if (address >= 0x0400 && address <= 0x07FF) _data = _vRAM[address & 0x03FF];
					if (address >= 0x0800 && address <= 0x0BFF) _data = _vRAM[0x03FF + (address & 0x03FF)];
					if (address >= 0x0C00 && address <= 0x0FFF) _data = _vRAM[0x03FF + (address & 0x03FF)];
				}
			break;

			case 3: // $3F00-3FFF -> Palette RAM
				return _palette_RAM[get_palette_ram_address(address)];
			
			default: break;
		}

		return 0;
	}
}