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
			/// Interrupt Handler
			_ram->write(0x0700, 0xA9); // LDA ZP
			_ram->write(0x0701, 0x02); // data
			_ram->write(0x0702, 0x38); // data -> for normal subtraction
			_ram->write(0x0703, 0xE9); // SBC
			_ram->write(0x0704, 0x04); // data
			_ram->write(0x0705, 0xA9); // LDA ZP
			_ram->write(0x0706, 0x8D); // data
			_ram->write(0x0707, 0x40); // RTI
			/// END

			/// Subroutine
			_ram->write(0x0730, 0xA9); // LDA
			_ram->write(0x0731, 0x40); // data
			_ram->write(0x0732, 0xE6); // INC ZP
			_ram->write(0x0733, 0x10); // data
			_ram->write(0x0734, 0xA6); // LDX ZP
			_ram->write(0x0735, 0x10); // data
			_ram->write(0x0736, 0xD6); // DEC ZPX
			_ram->write(0x0737, 0x06); // data
			_ram->write(0x0738, 0xA9); // LDA IMM
			_ram->write(0x0739, 0x18); // data
			_ram->write(0x073A, 0xC9); // CMP
			_ram->write(0x073B, 0x17); // data
			_ram->write(0x073C, 0xA2); // LDX
			_ram->write(0x073D, 0x15); // data
			_ram->write(0x073E, 0xE0); // CPX
			_ram->write(0x073F, 0x18); // data
			_ram->write(0x0740, 0xA0); // LDY
			_ram->write(0x0741, 0xC1); // data
			_ram->write(0x0742, 0xC0); // CPY
			_ram->write(0x0743, 0xC0); // data
			_ram->write(0x0744, 0x60); // RTI
			/// END

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

			_ram->write(0x043A, 0x18); // CLC
			_ram->write(0x043B, 0x58); // CLI
			_ram->write(0x043C, 0xB8); // CLV
			_ram->write(0x043D, 0xD8); // CLD

			_ram->write(0x00FA, 0xFF); // data -> Bit Mask
			_ram->write(0x032C, 0xFA); // data
			_ram->write(0x043E, 0xA5); // LDA ZP
			_ram->write(0x043F, 0xFA); // data
			_ram->write(0x0440, 0x24); // BIT ZP0
			_ram->write(0x0441, 0x2C); // data
			_ram->write(0x0442, 0x2C); // BIT ABS
			_ram->write(0x0443, 0x2C); // data
			_ram->write(0x0444, 0x03); // data

			_ram->write(0x0445, 0xE8); // INX
			_ram->write(0x0446, 0xE8); // INX
			_ram->write(0x0447, 0xCA); // DEX
			_ram->write(0x0448, 0xC8); // INY
			_ram->write(0x0449, 0x88); // DEY

			_ram->write(0x00AB, 0xFD); // data
			_ram->write(0x044A, 0xA6); // LDX
			_ram->write(0x044B, 0xAB); // Data
			_ram->write(0x044C, 0x9A); // TXS

			_ram->write(0x044D, 0xEA); // NOP
			_ram->write(0x044E, 0x48); // PHA
			_ram->write(0x044F, 0x08); // PHP
			_ram->write(0x0450, 0x28); // PLP
			_ram->write(0x0451, 0x68); // PLA

			_ram->write(0x0454, 0x29); // AND
			_ram->write(0x0455, 0x68); // data
			_ram->write(0x0456, 0x49); // EOR
			_ram->write(0x0457, 0x68); // data
			_ram->write(0x0458, 0x09); // ORA
			_ram->write(0x0459, 0x0F); // data

			_ram->write(0x045A, 0x20); // JSR
			_ram->write(0x045B, 0x30); // low
			_ram->write(0x045C, 0x07); // high

			_ram->write(0x045D, 0x09); // ORA
			_ram->write(0x045E, 0x0A); // data

			_ram->write(0x045F, 0x0A); // ASL
			_ram->write(0x0460, 0x4A); // LSR

			_ram->write(0x0461, 0x2A); // ROL
			_ram->write(0x0462, 0x38); // SEC
			_ram->write(0x0463, 0x2A); // ROL

			_ram->write(0x0464, 0x18); // CLC
			_ram->write(0x0465, 0x6A); // ROR
			_ram->write(0x0466, 0x38); // SEC
			_ram->write(0x0467, 0x6A); // ROR

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