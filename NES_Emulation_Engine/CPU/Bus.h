#pragma once

#include "../Common/CommonHeaders.h"
#include "../Memory/RAM.h"
#include "../PPU/R2C02.h"
#include "../Cartridge/Cartridge.h"
#include "../Controller/Controller.h"
#include "../Common/CpuTest.h"

namespace NES::CPU {
	// When the CPU attempts to read from an address which has no devices active, the result is open bus behavior.
	class Bus {
	public:
		Bus() {
			_cartridge = Cartridge::load_file("C:/Users/shrey/source/repos/NES_Emulator/x64/Debug/test.nes");
			_cartridge_inserted = true;

			_ppu.connect_card(_cartridge);

			_controller[0] = std::make_shared<Input::Standard>(0);
			_controller[1] = std::make_shared<Input::Standard>(1);
		}

		~Bus() { /* Delete Pointers */ }

		void reset() {
			_ram.reset();
			_ppu.reset();

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
			TEST_INTERRUPT_HANDLER
			TEST_SUBROUTINE
			TEST_PROGRAM_A
			TEST_PROGRAM_B
			TEST_PROGRAM_BRANCH
#endif // CPU_TEST

#if PPU_TEST

#endif // PPU_TEST

		}

		// Control Bus Function -> To signal if the cpu is reading or writing

		void set_cartridge_inserted(bool value) { _cartridge_inserted = value; }

		void disassembleRAM() { _ram.disassemble_wram(); }
		void disassembleRAM(u32 start, u32 end) { // Disassembler - [Start, End)
			_ram.disassemble_wram(start, end); 
		}

		bool clock(bool &dma_trigger) {
			dma_trigger = _dma_enabled;
			_ppu.clock();
			if (_ppu._nmi_trigger) {
				_ppu._nmi_trigger = false;
				return true; // it might go into nmi at different times, making inconsistent clock routines
			}
			return false;
		}

		void dma_read() {
			_dma_data = read(_dma_page << 8 | _dma_address);
		}

		u8 dma_write() {
			_ppu._OAM_pointer[_dma_address] = _dma_data;
			return ++_dma_address;
		}

		void dma_disable() { _dma_enabled = false; }

		void get_ppu(PPU::R2C02*& ppu) {
			ppu = &_ppu;
		}

		void get_controllers(std::array<std::shared_ptr<NES::Input::Controller>, 2>& controller) {
			controller = _controller;
		}

		// Writes Data to the Address Location on the Bus
		void write(u16 address, u8 data);
		// Reads Data from the Address Location on the Bus
		[[nodiscard]]u8 read(u16 address, bool bReadOnly = false);

		u8 controller[2];
	private:
		// Instance or whatever data is needed by CPU/PPU from the cartridge
		bool														_cartridge_inserted{ false };
		std::shared_ptr<NES::Cartridge::GameCard>					_cartridge;

		// I/O Hardware
		NES::PPU::R2C02												_ppu{};
		NES::Memory::RAM											_ram{};
		std::array<std::shared_ptr<NES::Input::Controller>, 2>		_controller;

		// OAM stuff
		u8															_dma_page{ 0x00 };
		u8															_dma_address{ 0x00 };
		u8															_dma_data{ 0x00 };

		bool														_dma_enabled{ false };

		u8 controller_state[2];
	};

} // NES CPU