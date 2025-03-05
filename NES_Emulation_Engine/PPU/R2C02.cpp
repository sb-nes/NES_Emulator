#include "R2C02.h"

namespace NES::PPU { // [Picture Processing Unit]
	namespace {

		u16 get_cpu_address(u16 m_address) { // if the address points to a mirror of the PPU Address Bus, bitwise modulo gets the address. -> |x % 2^n| = x & (2^n -1) -> Only works for powers of 2
			return m_address & 0x0007; // Size of CPU/PPU Address Bus is half a byte
		}

		u16 get_address(u16 m_address) { // if the address points to a mirror of the PPU Address Bus, bitwise modulo gets the address. -> |x % 2^n| = x & (2^n -1) -> Only works for powers of 2
			return m_address & 0x3FFF; // Size of PPU Address Bus is 16*1024 bytes | 16KB
		}

	} // Anonymous Namespace

	// Writes to the Address Bus
	void R2C02::cpubus_write(u16 address, u8 data) {
		switch (get_cpu_address(address)) {

		case 0x0000: _ctrl_register.value = data; break; // PPUCTRL -> Control
		case 0x0001: _mask_register.value = data; break; // PPUMASK -> Mask
		case 0x0002: break; // PPUSTATUS -> Status
		case 0x0003: break; // OAMADDR -> [Object Attribute Memory] OAM address
		case 0x0004: break; // OAMDATA -> [Object Attribute Memory] OAM data
		case 0x0005: break; // PPUSCROLL -> Scroll

			case 0x0006: // PPUADDR -> [Picture Processing Unit] Memory Address
				if (_address_latch == 0) {
					_address_abs = (_address_abs & 0xFF00) | data;
					_address_latch = 1;
				}
				_address_abs = (_address_abs & 0x00FF) | (data<<8);
				_address_latch = 0;
			break; 

			case 0x0007: // PPUDATA -> [Picture Processing Unit] Memory Data
				write(_address_abs, data);
			break; 

			default:
			break;
		}
	}
	// Reads from the Address Bus
	u8 R2C02::cpubus_read(u16 address, bool bReadOnly) {
		u8 data = 0x00;

		switch (get_cpu_address(address)) {
			
		case 0x0000: break; // PPUCTRL -> Control
		case 0x0001: break; // PPUMASK -> Mask
		case 0x0002: break; // PPUSTATUS -> Status
		case 0x0003: break; // OAMADDR -> [Object Attribute Memory] OAM address
		case 0x0004: break; // OAMDATA -> [Object Attribute Memory] OAM data
		case 0x0005: break; // PPUSCROLL -> Scroll

			case 0x0006: // PPUADDR -> [Picture Processing Unit] Memory Address
				// Can't read from the address register
			break; 

			case 0x0007: // PPUDATA -> [Picture Processing Unit] Memory Data

			break; 
		
			default:
			break;
		}

		return 0;
	}

	// Writes to the PPU's Address Bus
	void R2C02::write(u16 address, u8 data) {
		address = get_address(address);

	}

	// Reads from the PPU's Address Bus
	u8 R2C02::read(u16 address, bool bReadOnly) {
		u8 data = 0x00;
		address = get_address(address);

		return 0;
	}


}