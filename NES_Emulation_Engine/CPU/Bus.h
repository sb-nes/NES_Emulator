#pragma once

#include "../Common/CommonHeaders.h"
#include "../Memory/RAM.h"
#include "../PPU/R2C02.h"
#include "../Cartridge/Cartridge.h"
// #include "R6502.h"

namespace NES::CPU {
	// When the CPU attempts to read from an address which has no devices active, the result is open bus behavior.
	class Bus {
	public:
		Bus() {
			_ram = new NES::Memory::RAM();
			_ppu = new NES::PPU::R2C02();
		}

		~Bus() { // Delete Pointers
			delete _ram;
			delete _ppu;
		}

		void reset() {

#if RAM_TEST
			_ram->write(0x0000, 0x76);
			_ram->write(0x0010, 0xB6);
			_ram->write(0x0015, 0x26);
			_ram->write(0x0201, 0x6B);
			_ram->write(0x07F1, 0x6B);
			_ram->write(0x07FF, 0xFF);
			_ram->disassemble_wram();
#endif // RAM_TEST

#if CPU_TEST
			_ram->write(0x0000, 0xA9); // LDA
			_ram->write(0x0001, 0x32); // 50
			_ram->disassemble_wram();
#endif // CPU_TEST


		}

		// Control Bus Function -> To signal if the cpu is reading or writing

		void set_cartridge_inserted(bool value) { _cartridge_inserted = value; }

		// Writes Data to the Address Location on the Bus
		void write(u16 address, u8 data);
		// Reads Data from the Address Location on the Bus
		[[nodiscard]]u8 read(u16 address, bool bReadOnly = false);


	private:

		// R6502 _cpu;
		// Instance or whatever data is needed by PPU from the cartridge
		bool										_cartridge_inserted{ false };
		std::shared_ptr<NES::Cartridge::GameCard>	_cartridge;

		// I/O Registers
		NES::PPU::R2C02*							_ppu;
		NES::Memory::RAM*							_ram;

	};

} // NES CPU