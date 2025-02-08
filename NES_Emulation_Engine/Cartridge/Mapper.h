#pragma once
#include "../Common/CommonHeaders.h"

namespace NES::Cartridge {
	class Mapper { // Abstract class as blueprint for other classes
	public:
		Mapper(u8 prg_banks, u8 chr_banks) : _program_banks_count{ prg_banks }, _character_banks_count{ chr_banks } {}

		// Read/Write Functions, which transforms the address into the cartridge rom's address space.
		virtual bool cpuMapRead(u16 address, u32 &mapped_address) = 0;
		virtual bool cpuMapWrite(u16 address, u32 &mapped_address) = 0;
		virtual bool ppuMapRead(u16 address, u32 &mapped_address) = 0;
		virtual bool ppuMapWrite(u16 address, u32 &mapped_address) = 0;

		[[nodiscard]] constexpr u8 get_program_banks_count() { return _program_banks_count; }
		[[nodiscard]] constexpr u8 get_character_banks_count() { return _character_banks_count; }

		

	private:
		const u8					_program_banks_count;
		const u8					_character_banks_count;
	};
}