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
			_ram->write(0x0010, 0x02); // data
			_ram->write(0x0015, 0x08); // data
			_ram->write(0x0000, 0xA9); // LDA immediate
			_ram->write(0x0001, 0x32); // 50
			_ram->write(0x0002, 0xA5); // LDA zero page
			_ram->write(0x0003, 0x10); // $0010

			_ram->write(0x0004, 0x69); // ADC immediate
			_ram->write(0x0005, 0x15); // $0010
			_ram->write(0x0006, 0x6D); // ADC Absolute
			_ram->write(0x0007, 0x15); // $0010
			_ram->write(0x0008, 0x00); // $0010

			_ram->write(0x0009, 0xB4); // LDY ZP
			_ram->write(0x000A, 0x10); // $0010
			_ram->write(0x000B, 0xA0); // LDX IMM
			//_ram->write(0x000B, 0xB6); // LDX ZPY
			_ram->write(0x000C, 0x03); // $0010
			
			_ram->write(0x000D, 0xA0); // LDY ZPX
			//_ram->write(0x000D, 0xB4); // LDY ZPX
			//_ram->write(0x000D, 0xB5); // LDA ZPX
			_ram->write(0x000E, 0x03); // $0010

			_ram->write(0x000D, 0x6C); // JMP ABS
			_ram->write(0x000E, 0x0E); // $0010
			_ram->write(0x000F, 0x01); // $0010

			_ram->write(0x010E, 0x2D); // $0010
			_ram->write(0x010F, 0x04); // $0010

			//_ram->write(0x042D, 0xAE); // LDA ABS
			_ram->write(0x042D, 0xBE); // LDA ABY
			_ram->write(0x042E, 0xFE); // $0010
			_ram->write(0x042F, 0x01); // $0010

			_ram->write(0x0201, 0x6B); // Data
			_ram->write(0x0204, 0x5B); // Data

			_ram->write(0x0430, 0xBA); // TSX
			_ram->write(0x0431, 0xA5); // LDA ZP
			_ram->write(0x0432, 0x15); // Data
			_ram->write(0x0433, 0xAA); // TAX
			_ram->write(0x0434, 0x9A); // TXS

			_ram->write(0x0435, 0x85); // STA ZP
			_ram->write(0x0436, 0x00); // Data

			_ram->write(0x0437, 0x38); // SEC
			_ram->write(0x0438, 0xF8); // SED
			_ram->write(0x0439, 0x78); // SEI


#endif // CPU_TEST


		}

		// Control Bus Function -> To signal if the cpu is reading or writing

		void set_cartridge_inserted(bool value) { _cartridge_inserted = value; }

		void disassembleRAM() { _ram->disassemble_wram(); }
		void disassembleRAM(u32 start, u32 end) { // Disassembler - [Start, End)
			_ram->disassemble_wram(start, end); 
		}

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