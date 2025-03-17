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

		R2C02() { }

		~R2C02() { }

		// CPU Address BUS read and write:

		// Writes to the Address Bus
		void cpubus_write(u16 address, u8 data);
		// Reads from the Address Bus
		u8 cpubus_read(u16 address, bool bReadOnly = false);

		[[nodiscard]] sprite_tile get_tile_at_address(u16 address, u8 palette_idx);
		[[nodiscard]] pattern_table get_pattern_table(u8 pattern_table_idx, u8 palette_idx);
		[[nodiscard]] palette get_palette();
		
		u8* get_nametable() {
			return _bus.get_nametable();
		}

		bool clock() {
			// Clock function of the PPU

			if (_scanline == -1 && _cycle == 1) {
				_status_register.v_blank = 0;
			}
				
			if (_scanline >= 241 && _cycle == 1) {
				_status_register.v_blank = 1;
				if (_ctrl_register.nmi_enable) _nmi_trigger = true;
			}

			++_cycle; // Works like a scanline across the screen of the CRT
			if (_cycle >= 341) { // HIT CRT EDGE
				_cycle = 0;
				++_scanline;
				if (_scanline >= 261) { // Past V-Blank Space
					_scanline = -1;
					_frame_scan_complete = true;
				}
			}

			// TODO: implement rendering here

			return _nmi_trigger;
		}

		void reset() {
			_status_register.value = 0x00;
			_mask_register.value = 0x00;
			_ctrl_register.value = 0x00;
			_address_abs = 0x0000;
			_address_inc = 0x0000;
		}

		void connect_card(std::shared_ptr<NES::Cartridge::GameCard> card) {
			_bus.connect_card(card);
		}

	private:
		PPU_Bus		_bus{};

		u8			_address_latch{ 0x00 }; // writing to the Low byte or the High byte
		u8			_ppu_read_buffer{ 0x00 }; // since, reading data from ppu is delayed by 1 cycle

		u16			_address_abs{ 0x0000 };
		u16			_address_inc{ 0x0000 };
		s16			_scanline{ 0 };
		s16			_cycle{ 0 };

		bool		_frame_scan_complete{ false };

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