#pragma once

#include "../Common/CommonHeaders.h"
#include "../Cartridge/Cartridge.h"

namespace NES::PPU {
	class PPU_Bus {
	public:
	private:
		// Instance or whatever data is needed by PPU from the cartridge
		std::shared_ptr<NES::Cartridge::GameCard>	_card;

		// CHR-ROM/CHR-RAM - $0000-$1FFF -> from the card, using Bank Switching
		// Contains Pattern Table
		u8		_pattern_table[2][4096]; // Exist on the cartridge -> TODO: shift to cartridge

		// VRAM -> 2KB
		u8		_vRAM[2][1024]; // $2000-$2FFF | Mirrors of _VRAM -> $3000-$3EFF

		// Palette RAM indexes + Mirrors -> $3F00-$3F1F + $3F20-$3FFF
		u8		_palette_RAM[32];

		// Object Attribute Memory [OAM]
	};
}