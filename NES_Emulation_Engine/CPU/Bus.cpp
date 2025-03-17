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
		assert(address != 0x8153);

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

				}
			break; 

			case 3: // $6000 Cartridge
				_cartridge->cpu_write(address, data);
			break;

			default: break;
		}

		/*
		if (_cartridge->cpu_write(address, data))
		{
			// The cartridge "sees all" and has the facility to veto
			// the propagation of the bus transaction if it requires.
			// This allows the cartridge to map any address to some
			// other data, including the facility to divert transactions
			// with other physical devices. The NES does not do this
			// but I figured it might be quite a flexible way of adding
			// "custom" hardware to the NES in the future!
		}
		else if (address >= 0x0000 && address <= 0x1FFF)
		{
			// System RAM Address Range. The range covers 8KB, though
			// there is only 2KB available. That 2KB is "mirrored"
			// through this address range. Using bitwise AND to mask
			// the bottom 11 bits is the same as addr % 2048.
			_cpuRam[address & 0x07FF] = data;

		}
		else if (address >= 0x2000 && address <= 0x3FFF)
		{
			// PPU Address range. The PPU only has 8 primary registers
			// and these are repeated throughout this range. We can
			// use bitwise AND operation to mask the bottom 3 bits, 
			// which is the equivalent of addr % 8.
			_ppu.cpubus_write(address & 0x0007, data);
		}
		else if (address >= 0x4016 && address <= 0x4017)
		{
			_controller_state[address & 0x0001] = _controller[address & 0x0001];
		}
		*/
	}

	// Reads from the Address Bus
	u8 Bus::read(u16 address, bool bReadOnly) {
		assert(address >= 0x0000 && address <= 0xFFFF);
		assert(address != 0x8153);

		switch (chip_select(address)) {
			// $0000 SRAM/WRAM
			case 0: return _ram.read(address);

			case 1: // $2000-$0x3FFF PPU
				return _ppu.cpubus_read(address);

			case 2: // $4000 I/O Registers + Cartridge
				if (chip_select_4000(address)) // $4020-5FFF Cartridge
					return _cartridge->cpu_read(address);
				else { // $4000-401F I/O Registers

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

		/*
		uint8_t data = 0x00;
		if (_cartridge->cpu_read(address, data))
		{
			// Cartridge Address Range
		}
		else if (address >= 0x0000 && address <= 0x1FFF)
		{
			// System RAM Address Range, mirrored every 2048
			data = _cpuRam[address & 0x07FF];
		}
		else if (address >= 0x2000 && address <= 0x3FFF)
		{
			// PPU Address range, mirrored every 8
			data = _ppu.cpubus_read(address & 0x0007, bReadOnly);
		}
		else if (address >= 0x4016 && address <= 0x4017)
		{
			data = (_controller_state[address & 0x0001] & 0x80) > 0;
			_controller_state[address & 0x0001] <<= 1;
		}

		return data;
		*/
	}
}