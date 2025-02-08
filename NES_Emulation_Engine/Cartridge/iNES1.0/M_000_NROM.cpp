
#include "M_000_NROM.h"

namespace NES::Cartridge {
	namespace {

		[[nodiscard]]constexpr u16 map_cpu_to_cartridge(u16 address, u8 banks) {
			return address & (banks > 1 ? 0x7FFF : 0x3FFF); // More than 1 bank -> 32KB ROM | 16KB ROM is mirrored
		}

	} // anonymous namespace

	bool NROM::cpuMapRead(u16 address, u32& mapped_address) {
		if (address >= 0x8000 && address <= 0xFFFF) { // Dedicated Address Space For Cartridge Use
			mapped_address = map_cpu_to_cartridge(address, get_program_banks_count());
			return true;
		}
		return false;
	}
	bool NROM::cpuMapWrite(u16 address, u32& mapped_address) {
		if (address >= 0x8000 && address <= 0xFFFF) { // Dedicated Address Space For Cartridge Use
			// ???
			return true;
		}
		return false;
	}
	bool NROM::ppuMapRead(u16 address, u32& mapped_address) {
		if (address >= 0x0000 && address <= 0x1FFF) { // Pattern Tables
			mapped_address = address;
			return true;
		}
		return false;
	}
	bool NROM::ppuMapWrite(u16 address, u32& mapped_address) {
		//if (address >= 0x0000 && address <= 0x1FFF) { // Pattern Tables
		//
		//	return true;
		//}
		return false;
	}
}