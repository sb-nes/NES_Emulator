#pragma once

#include "../Common/CommonHeaders.h"
#include "PPU_Bus.h"

// 1 cycle = 1 pixel
// 341 cycles per scanline
// 261 scanlines for NTSC, 312 scanlines for PAL
// Vertical Blank Period -> when scanlines go past screen height of 240(PAL)/224(NTSC)

// During the vertical blank period, the cpu is setting up the ppu for the next frame
// After the last scanline, it jumps to -1 scanline instead of +0 scanline

namespace NES::PPU { // Picture Processing Unit
	class R2C02 {
	public:
		bool		_nmi_trigger{ false };
		bool		_frame_scan_complete{ false };

		R2C02() { reset(); }

		~R2C02() { }

		// CPU Address BUS read and write:

		// Writes to the Address Bus
		void cpubus_write(u16 address, u8 data);
		// Reads from the Address Bus
		u8 cpubus_read(u16 address, bool bReadOnly = false);

		[[nodiscard]] sprite_tile get_tile_at_address(u16 address, u8 palette_idx);
		[[nodiscard]] pattern_table get_pattern_table(u8 pattern_table_idx, u8 palette_idx);
		[[nodiscard]] nametable get_nametable(u8 nametable_idx);
		[[nodiscard]] palette get_palette();
		[[nodiscard]] display get_render_screen();

		bool clock() {
			// Clock function of the PPU

			// TODO: implement rendering here

			auto IncrementScrollX = [&]() {
				if (_mask_register.background_enable || _mask_register.sprite_enable) {
					if (_v_register.coarse_x == 31) { // End of Nametable
						_v_register.coarse_x = 0;
						_v_register.nametable_x = ~(_v_register.nametable_x); // but why flip?
					} else {
						++_v_register.coarse_x;
					}
				}
			};

			auto IncrementScrollY = [&]() {
				if (_mask_register.background_enable || _mask_register.sprite_enable) {
					if (_v_register.fine_y < 7) {
						++_v_register.fine_y;
					} else {
						_v_register.fine_y = 0;

						if (_v_register.coarse_y == 29) { // End of Nametable
							_v_register.coarse_y = 0;
							_v_register.nametable_y = ~(_v_register.nametable_y); // again, why flip?
						} else if (_v_register.coarse_y == 31) { // Attribute Mem. just in case
							_v_register.coarse_y = 0;
						} else {
							++_v_register.coarse_y;
						}
					}
				}
			};

			auto ResetAddressX = [&]() {
				if (_mask_register.background_enable || _mask_register.sprite_enable) {
					_v_register.nametable_x = _t_register.nametable_x;
					_v_register.coarse_x = _t_register.coarse_x;
				}
			};

			auto ResetAddressY = [&]() {
				if (_mask_register.background_enable || _mask_register.sprite_enable) {
					_v_register.fine_y = _t_register.fine_y;
					_v_register.nametable_y = _t_register.nametable_y;
					_v_register.coarse_y = _t_register.coarse_y;
				}
			};

			auto LoadBgShiftRegisters = [&]() { // Prepapres the registers

				bg_shifter_pattern_lo = (bg_shifter_pattern_lo & 0xFF00) | _bitplane_lsb_background;
				bg_shifter_pattern_hi = (bg_shifter_pattern_hi & 0xFF00) | _bitplane_msb_background;

				bg_shifter_attrib_lo = (bg_shifter_attrib_lo & 0xFF00) | ((_attribute_background & 0b01) ? 0xFF : 0x00);
				bg_shifter_attrib_hi = (bg_shifter_attrib_hi & 0xFF00) | ((_attribute_background & 0b10) ? 0xFF : 0x00);
			};

			auto UpdateShiftRegisters = [&]() {
				if (_mask_register.background_enable) {
					bg_shifter_pattern_lo <<= 1;
					bg_shifter_pattern_hi <<= 1;

					bg_shifter_attrib_lo <<= 1;
					bg_shifter_attrib_hi <<= 1;
				}
			};

			if (_scanline >= -1 && _scanline < 240) {

				if (_scanline == -1 && _cycle == 1) { // Pre-Render Scanline
					_status_register.v_blank = 0;
				} 

				if (_scanline == 0 && _cycle == 0) { // Odd Frame
					++_cycle; // Skipped on BG+Odd
				}

				if ((_cycle >= 2 && _cycle < 258) || (_cycle >= 321 && _cycle < 338)) {
					UpdateShiftRegisters();

					// extract tile id, attribute and bitmap patterns
					switch ((_cycle - 1) % 8) {
						case 0: // NT / Nametable Read
							LoadBgShiftRegisters();
							_pattern_id_background = read(0x2000 | (_v_register.value & 0x0FFF));
						break;

						case 2: // AT / Attribute table Read
							// Simple Explanation: Using bit manipulation to divide coarse_y and coarse_x by 4, such that we can look at regions of '16x16' instead of 8x8 tiles
							_attribute_background = read((_v_register.nametable_y << 11) | (_v_register.nametable_x << 10) | ((_v_register.coarse_y >> 2) << 3) | (_v_register.coarse_x >> 2) | 0x23C0);
							
							// Bottom-Right << 6 | Bottom-Left << 4 | Top-Right << 2 | Top-Left 
							if (_v_register.coarse_y & 0x02) _attribute_background >>= 4; // Top or Bottom
							if (_v_register.coarse_x & 0x02) _attribute_background >>= 2; // Left or Right
							_attribute_background &= 0x03; // Only need the last two bits
						break;

						case 4:
							_bitplane_lsb_background = read((_ctrl_register.background_tile_select << 12) + ((u16)_pattern_id_background << 4) + _v_register.fine_y);
						break;

						case 6:
							_bitplane_msb_background = read((_ctrl_register.background_tile_select << 12) + ((u16)_pattern_id_background << 4) + _v_register.fine_y + 8);
						break;

						case 7:
							IncrementScrollX();
						break;

						default: break;
					}
				}

				if (_cycle == 256) { // End of Scanline
					// Increment the loopy register in y direction
					IncrementScrollY();
				}

				if (_cycle == 257) { // Somewhere
					LoadBgShiftRegisters();
					ResetAddressX();
				}

				if (_cycle == 338 || _cycle == 340) {
					_pattern_id_background = read(0x2000 | (_v_register.value & 0x0FFF));
				}

				if (_scanline == -1 && _cycle >= 280 && _cycle < 305) { // Where???
					// End of vertical blank period so reset the Y address ready for rendering
					ResetAddressY();
				}
			}
				
			if (_scanline == 240) { // Post-render scanline
				// Do Nothing RN
			}

			if (_scanline >= 241 && _scanline < 261) {
				// error 1: i was triggering nmi for every scanline after vertical blank starts
				if (_scanline == 241 && _cycle == 1) { // Vertical blanking lines
					_status_register.v_blank = 1;
					if (_ctrl_register.nmi_enable) _nmi_trigger = true;
				}
			}

			
			u8 bg_pix{ 0x00 };
			u8 bg_pal{ 0x00 };

			if (_mask_register.background_enable) {
				u16 bit_mux = 0x8000 >> _x_register;

				u8 p0_pixel = (bg_shifter_pattern_lo & bit_mux) > 0;
				u8 p1_pixel = (bg_shifter_pattern_hi & bit_mux) > 0;
				bg_pix = (p1_pixel << 1) | p0_pixel;

				u8 bg_pal0 = (bg_shifter_attrib_lo & bit_mux) > 0;
				u8 bg_pal1 = (bg_shifter_attrib_hi & bit_mux) > 0;
				bg_pal = (bg_pal1 << 1) | bg_pal0;
			}

			// set pixel
			if (_scanline > 0 && _scanline <= 240 && _cycle >= 0 && _cycle <= 255) {
				_display[_scanline-1][_cycle] = _bus.read_palette_colour(bg_pal, bg_pix);
			}

			++_cycle; // Scans Across the Screen

			if (_cycle >= 341) { // HIT CRT EDGE
				_cycle = 0;
				++_scanline; // Scans Vertically Down
				if (_scanline >= 261) { // Past V-Blank Space
					_scanline = -1;
					_frame_scan_complete = true;
				}
			}

			return _nmi_trigger;
		}

		void reset() {
			// MMIO Registers
			_status_register.value = 0x00;
			_mask_register.value = 0x00;
			_ctrl_register.value = 0x00;
			// Internal Registers
			_t_register.value = 0x0000;
			_v_register.value = 0x0000;
			_x_register = 0x00;
			_w_register = 0x00;

			_internal_read_buffer = 0x00;
			_scanline = 0;
			_cycle = 0;

			_pattern_id_background = 0x00;
			_attribute_background = 0x00;
			_bitplane_lsb_background = 0x00;
			_bitplane_msb_background = 0x00;

			// Shift Registers
			bg_shifter_pattern_lo = 0x0000;
			bg_shifter_pattern_hi = 0x0000;
			bg_shifter_attrib_lo = 0x0000;
			bg_shifter_attrib_hi = 0x0000;
		}

		void connect_card(std::shared_ptr<NES::Cartridge::GameCard> card) {
			_bus.connect_card(card);
		}

	private:

		// as described by loopy | original src - unknown?
		union address_register {
			struct {
				u16 coarse_x : 5;
				u16 coarse_y : 5;
				u16 nametable_x : 1;
				u16 nametable_y : 1;
				u16 fine_y : 3;
				u16 unused : 1;
			};

			u16 value = 0x0000;
		};
		
		PPU_Bus					_bus{};

		address_register		_v_register; // Address | Scroll Position
		address_register		_t_register;
		u8						_x_register{ 0x00 }; // fine_x
		u8						_w_register{ 0 }; // Write Latch/Toggle | part of PPUSCROLL
		u8						_internal_read_buffer{ 0x00 }; // since, reading data from ppu is delayed by 1 cycle

		u16						_address_internal{ 0x0000 };
		s16						_scanline{ 0 };
		s16						_cycle{ 0 };


		u8						_pattern_id_background{ 0x00 };
		u8						_attribute_background{ 0x00 };
		u8						_bitplane_lsb_background{ 0x00 }; // for bit-planes?
		u8						_bitplane_msb_background{ 0x00 }; // for bit-planes?

		// 16-bit Shift Registers
		u16						bg_shifter_pattern_lo{ 0x0000 };
		u16						bg_shifter_pattern_hi{ 0x0000 };
		u16						bg_shifter_attrib_lo{ 0x0000 };
		u16						bg_shifter_attrib_hi{ 0x0000 };
		// It is a mouthful, IK...

		display					_display{};

		union {
			struct
			{
				u8 nametable_select_x : 1; // N
				u8 nametable_select_y : 1; // N
				u8 increment_mode : 1; // I
				u8 sprite_tile_select : 1; // S
				u8 background_tile_select : 1; // B
				u8 sprite_height : 1; // H
				u8 ppu_master_slave_mode : 1; // P
				u8 nmi_enable : 1; // V
			};

			u8 value;
		} _ctrl_register;

		union {
			struct
			{
				u8 grayscale : 1; // G
				u8 background_left_column_enable : 1; // m
				u8 sprites_left_column_enable : 1; // M
				u8 background_enable: 1; // b
				u8 sprite_enable: 1; // s
				u8 colour_emphasis_red: 1; // R
				u8 colour_emphasis_green : 1; // G
				u8 colour_emphasis_blue : 1; // B
			};

			u8 value;
		} _mask_register;

		union {
			struct {
				u8 unused : 5; // allocate 5 bits using bit-fields | read resets write pair for $2005/$2006
				u8 sprite_overflow : 1; // allocate 1 bit | O
				u8 sprite_0_hit : 1; // allocate 1 bit | S
				u8 v_blank : 1; // allocate 1 bit | V -> Screen Space[0] or Vertical Blank Space[1]
			};

			u8 value;
		} _status_register;
		
		// Writes to the PPU's Address Bus
		void write(u16 address, u8 data);
		// Reads from the PPU's Address Bus
		u8 read(u16 address, bool bReadOnly = false);

	};
}