#pragma once

#include "../Common/CommonHeaders.h"

// https://www.nesdev.org/wiki/CPU_memory_map
// Some parts of the 2 KiB of internal RAM at $0000–$07FF have predefined purposes dictated by the 6502 architecture:
// $0000 - $00FF: The zero page, which can be accessed with fewer bytes and cycles than other addresses.
// $0100–$01FF : The page containing the stack, which can be located anywhere here, but typically starts at $01FF and grows downward.

namespace NES::Memory {
	namespace {
		
		u16 get_address(u16 m_address) { // if the address points to a mirror of the ram, bitwise modulo gets the address. -> |x % 2^n| = x & (2^n -1) -> Only works for powers of 2
			return m_address & 0x07FF; // Size of RAM is 2048 bytes | 2KB
		}

	} // anonymous namespace

	class RAM {
	public:
		RAM() {
			for (auto& i : _ram) i = 0x00; // Clear RAM before creating, Just in Case...

			for (int i{ 0 }; i < 2048; ++i) { // u8 makes it ill defined... [Just Warning tho]
				_ram_heap[i] = 0x00; // Clear RAM before creating, Just in Case...
			}
		}

		~RAM() {
			delete[] _ram_heap;
		}

		u8 read(u16 address, bool bReadOnly = false) {
			return _ram[get_address(address)];
		}

		void write(u16 address, u8 data) {
			assert(address >= 0x0000 && address <= 0x1FFF); // RAM and it's mirrors
			assert(data >= 0x00 && data <= 0xFF); // Is this check necessary?

			_ram[get_address(address)] = data;
		}

		void disassemble_wram();

	private:
		// WRAM - Work RAM -> 2KB Static RAM [SRAM]

		std::array<u8, 2048> _ram{}; // Should I use the stack or heap?
		u8* const _ram_heap = new u8[2048];
	};
}