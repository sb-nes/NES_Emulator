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
			case 0x0000: // PPUCTRL -> Control
				_ctrl_register.value = data; 
			break; 

			case 0x0001: // PPUMASK -> Mask
				_mask_register.value = data; 
			break; 

			case 0x0002: // PPUSTATUS -> Status
			break; 

		case 0x0003: break; // OAMADDR -> [Object Attribute Memory] OAM address
		case 0x0004: break; // OAMDATA -> [Object Attribute Memory] OAM data
		case 0x0005: break; // PPUSCROLL -> Scroll

			case 0x0006: // PPUADDR -> [Picture Processing Unit] Memory Address
				if (_address_latch == 0) { // store high address
					_address_inc = (_address_inc & 0x00FF) | ((data & 0x3F) << 8);
					_address_latch = 1;
				} else { // store low address
					_address_inc = (_address_inc & 0xFF00) | data;
					_address_abs = _address_inc;
					_address_latch = 0;
				}
			break; 

			case 0x0007: // PPUDATA -> [Picture Processing Unit] Memory Data
				write(_address_abs, data);
				++_address_abs;
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

			case 0x0002: // PPUSTATUS -> Status
#if PPU_TEST
				_status_register.v_blank = 1; // for testing purposes | OLC's method
#endif // PPU_TEST

				data = (_status_register.value & 0xE0) | (_ppu_read_buffer & 0x1F);
				_status_register.v_blank = 0; // reading clears Vertical Blank Flag
				_address_latch = 0; // reset address read latch
			break; 

		case 0x0003: break; // OAMADDR -> [Object Attribute Memory] OAM address
		case 0x0004: break; // OAMDATA -> [Object Attribute Memory] OAM data
		case 0x0005: break; // PPUSCROLL -> Scroll

			case 0x0006: // PPUADDR -> [Picture Processing Unit] Memory Address
				// Can't read from the address register
			break; 

			case 0x0007: // PPUDATA -> [Picture Processing Unit] Memory Data
				data = _ppu_read_buffer; // PPU bus reads are too slow and cannot complete in time to service the CPU read. Thus, it has an internal buffer for storing data to be delivered later.
				_ppu_read_buffer = read(_address_abs);

				if (_address_abs > 0x3F00) data = _ppu_read_buffer;
				++_address_abs;
			break;

			default:
			break;
		}

		return data;
	}

	// TODO: Never goes to write

	// Writes to the PPU's Address Bus
	void R2C02::write(u16 address, u8 data) {
		address = get_address(address);
		_bus->write(address, data);
	}

	// Reads from the PPU's Address Bus
	u8 R2C02::read(u16 address, bool bReadOnly) {
		u8 data = 0x00;
		address = get_address(address);
		return _bus->read(address);
	}

	sprite_tile R2C02::get_tile_at_address(u16 address, u8 palette_idx) {
		sprite_tile tile{};

		for (u8 i = 0; i < 8; ++i) {
			u8 tile_lsb = read(address + i); // Least Significant Bit of the Tile
			u8 tile_msb = read(address + 8 + i); // Most Significant Bit of the Tile
				for (u8 j = 0; j < 8; ++j) {
					u8 pixel_value = (tile_lsb & 0x01) + ((tile_msb & 0x01) << 1);
					// shift bit to the right
					tile_lsb >>= 1;
					tile_msb >>= 1;

					// get colour and store it
					tile[i][7-j] = _bus->read_palette_colour(palette_idx, pixel_value);
				}
		}

		return tile;
	}

	pattern_table R2C02::get_pattern_table(u8 pattern_table_idx, u8 palette_idx) {
		pattern_table table;
		table.resize(128);
		for (int i = 0; i < 128; ++i) {
			table[i].resize(128);
		}
		//table.push_back();

		for (u16 i = 0; i < 16; ++i) { // Y
			for (u16 j = 0; j < 16; ++j) { // X
				u16 offset = (i * 256) + (j * 16);
				u16 address = 0x1000 * pattern_table_idx + offset;

				for (u8 r = 0; r < 8; ++r) {
					u8 tile_lsb = R2C02::read(address + r); // Least Significant Bit of the Tile
					u8 tile_msb = R2C02::read(address + 8 + r); // Most Significant Bit of the Tile
					for (u8 c = 0; c < 8; ++c) {
						u8 pixel_value = (tile_lsb & 0x01) + ((tile_msb & 0x01) << 1);
						// shift bit to the right
						tile_lsb >>= 1;
						tile_msb >>= 1;

						// get colour and store it
						table[(i * 8) + r][(j * 8) + 7 - c] = _bus->read_palette_colour(palette_idx, pixel_value);
					}
				}
			}
		}

		return table;
	}

	palette R2C02::get_palette() {
		return _bus->get_palette_data();
	}

}