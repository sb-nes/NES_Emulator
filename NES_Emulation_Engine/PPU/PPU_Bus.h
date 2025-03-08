#pragma once

#include "../Common/CommonHeaders.h"
#include "../Memory/VRAM.h"
#include "../Cartridge/Cartridge.h"

namespace NES::PPU {
	class PPU_Bus {
	public:

		PPU_Bus() {
			reset();
		}

		u8 read_palette_colour(u8 palette_index, u8 colour_index) {
			assert(!(colour_index >> 2));
			return _palette_RAM[(palette_index * 4) + colour_index]; // could have used palette_index<<2 to multiply by 4
		}

		u8* get_palette_ram() {
			return _palette_RAM;
		}

		palette get_palette_data() {
			palette palette{};
			for (int i{ 0 }; i < 32; ++i) {
				palette[i] = _palette_RAM[i];
			}
			return palette;
		}

		void connect_card(std::shared_ptr<NES::Cartridge::GameCard> card) {
			_card = card;
		}

		void reset() {

			for (int i{ 0 }; i < 32; i) {
				_palette_RAM[i++] = 0x0F;
				_palette_RAM[i++] = 0x2D;
				_palette_RAM[i++] = 0x3D;
				_palette_RAM[i++] = 0x30;
			}
		}

		void write(u16 address, u8 data);
		[[nodiscard]] u8 read(u16 address, bool bReadOnly = false);

	private:
		// Instance or whatever data is needed by PPU from the cartridge
		std::shared_ptr<NES::Cartridge::GameCard> _card;

		// CHR-ROM/CHR-RAM - $0000-$1FFF -> from the card, using Bank Switching
		// Contains Pattern Table -> Sprites -> 8192 bytes broken into 2 tables
		// Each tile is broken into 2 bitplanes
		// Table 1 -> foreground elements
		// Table 2 -> background elements
		u8		_pattern_table[2][4096]; // Exist on the cartridge -> TODO: shift to cartridge

		// VRAM -> 2KB -> Nametable Memory
		u8		_vRAM[2][1024]; // $2000-$2FFF | Mirrors of _VRAM -> $3000-$3EFF

		// Palette RAM indexes + Mirrors -> $3F00-$3F1F + $3F20-$3FFF
		// Palette accesses the NES's main Palette of colours it can display -> range = 0x00-0x3F
		// 0x3F00 -> (Special) Background Colour -> Every Transparent Colour in all palettes
		// 0x3F01-0x3F04 -> Palette 1 and so on...
		// [0,1,2,3] - Background Palettes | [4,5,6,7] - Foreground Palettes
		u8		_palette_RAM[32];

		// Object Attribute Memory [OAM]

		u8		_data{ 0x00 };
	};
}