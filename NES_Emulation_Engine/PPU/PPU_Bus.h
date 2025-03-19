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

		palette get_palette_data() {
			palette palette{};
			for (int i{ 0 }; i < 32; ++i) {
				palette[i] = _palette_RAM[i];
			}
			return palette;
		}

		nametable get_nametable(u8 nametable_idx) {
			nametable table{};
			for (int i{ 0 }; i < 1024; ++i) {
				table[i] = _vRAM[nametable_idx][i];
			}
			return table;
		}

		void reset() {
#if PALETTE_TEST
			for (int i{ 0 }; i < 32; i) {
				_palette_RAM[i++] = 0x0F;
				_palette_RAM[i++] = 0x2D;
				_palette_RAM[i++] = 0x3D;
				_palette_RAM[i++] = 0x30;
			}
#else
			for (int i{ 0 }; i < 32; ++i) {
				_palette_RAM[i] = 0x3F;
			}
#endif // PALETTE_TEST
		}

		void write(u16 address, u8 data);
		[[nodiscard]] u8 read(u16 address, bool bReadOnly = false);

		u8* get_palette_ram() { return _palette_RAM; }
		void connect_card(std::shared_ptr<NES::Cartridge::GameCard> card) { _card = card; }

	private:

		// Palette RAM indexes + Mirrors -> $3F00-$3F1F + $3F20-$3FFF
		// Palette accesses the NES's main Palette of colours it can display -> range = 0x00-0x3F
		// 0x3F00 -> (Special) Background Colour -> Every Transparent Colour in all palettes
		// 0x3F01-0x3F04 -> Palette 1 and so on...
		// [0,1,2,3] - Background Palettes | [4,5,6,7] - Foreground Palettes

		u8												_palette_RAM[32];
		u8												_vRAM[2][1024]; // [Nametable + Attribute Table] x2
		// $2000-$2FFF -> VRAM -> 2KB -> Nametable Memory | Mirrors of _VRAM -> $3000-$3EFF | 
		std::shared_ptr<NES::Cartridge::GameCard>		_card;

		// Object Attribute Memory [OAM]
		u8		_data{ 0x00 };

		// Instance or whatever data is needed by PPU from the cartridge
		// ???
	};
}