// Bisqwit's NES Emulator
// 
// 
// Fill Buffer
// 

#pragma once
#include "../Common/CommonHeaders.h"
#include "../Utilities/RegBit.h"

namespace NES::Audio {

	static std::shared_ptr<NES::Cartridge::GameCard>					_cartridge_for_apu;

	static const u8 length_counters[32] = { 10, 254, 20, 2, 40,  8, 80,  6, 160,  8, 60, 10, 14, 12, 26, 14,
										   12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30 };

	static const u16 noise_periods[16] = { 2,4,8,16,32,48,64,80,101,127,190,254,381,508,1017,2034 };
	static const u16 dmc_periods[16] = { 428, 380, 340, 320, 286, 254, 226, 214, 190, 160, 142, 128, 106, 84, 72, 54 };

	static float sample_out[8196] = {0};
	static int sc{ 0 };
	static bool s_ready{ false };

	static std::ofstream audio_out;

	class APU {
	public:

		struct channel {
			int length_counter, linear_counter, address, envelope;
			int sweep_delay, env_delay, wave_counter, hold, phase, level;

			// SQR
			// TRI
			// NOISE
			// PCM/DMC


			union { // Per-Channel register file
				// $4000, $4004, $400C, $4012							// $4001, $4005, $4013								// $4002, $4006, $400A, $400E
				Utilities::RegBit<0, 8, u32> reg0;						Utilities::RegBit<8, 8, u32> reg1;					Utilities::RegBit<16, 8, u32> reg2;
				Utilities::RegBit<6, 2, u32> duty_cycle;				Utilities::RegBit<8, 3, u32> sweep_shift;			Utilities::RegBit<16, 4, u32> noise_freq;
				Utilities::RegBit<4, 1, u32> env_decay_disable;			Utilities::RegBit<11, 1, u32> sweep_decrease;		Utilities::RegBit<23, 1, u32> noise_type;
				Utilities::RegBit<0, 4, u32> env_decay_rate;			Utilities::RegBit<12, 3, u32> sweep_rate;			Utilities::RegBit<16, 11, u32> wave_length;
				Utilities::RegBit<5, 1, u32> env_decay_loop_enable;		Utilities::RegBit<15, 1, u32> sweep_enable;			// $4003, $4007, $400B, $400F, $4010
				Utilities::RegBit<0, 4, u32> fixed_volume;				Utilities::RegBit<8, 8, u32> pcm_length;			Utilities::RegBit<24, 8, u32> reg3;
				Utilities::RegBit<5, 1, u32> length_counter_disable;														Utilities::RegBit<27, 5, u32> length_counter_init;
				Utilities::RegBit<0, 7, u32> linear_counter_init;															Utilities::RegBit<30, 1, u32> loop_enabled;
				Utilities::RegBit<7, 1, u32> linear_counter_disable;														Utilities::RegBit<31, 8, u32> irq_enable;
			} reg;

			template<unsigned c>
			int tick(APU* apu) {
				channel& ch = *this;
				if (!apu->channels_enable[c]) return c == 4 ? 64 : 8;

				int wl = (ch.reg.wave_length + 1) * (c >= 2 ? 1 : 2);
				if (c == 3) wl = noise_periods[ch.reg.noise_freq];
				int volume = ch.length_counter ? ch.reg.env_decay_disable ? ch.reg.fixed_volume : ch.envelope : 0;

				// sample may change at wavelen intervals
				auto& S = ch.level;
				if (!apu->count(ch.wave_counter, wl)) return S;

				switch (c) {

				default: // SQR: with four different 8-step binary waveforms (32 bits of data total)
					if (wl < 8) return S = 8;
					return S = (0xF33C0C04U & (1U << (++ch.phase % 8 + ch.reg.duty_cycle * 8))) ? volume : 0;

				case 2: // TRI
					if (ch.length_counter && ch.linear_counter && wl >= 3) ++ch.phase;
					return S = (ch.phase & 15) ^ ((ch.phase & 16) ? 15 : 0);

				case 3: // Noise: Linear Feedback Shift Register
					if (!ch.hold) ch.hold = 1;
					ch.hold = (ch.hold >> 1) | (((ch.hold ^ (ch.hold >> (ch.reg.noise_type ? 6 : 1))) & 1) << 14);
					return S = (ch.hold & 1) ? 0 : volume;

				case 4: // Delta Modulation Channel
					// hold = 8-bit value, phase = no. of bits buffered

					if (ch.phase == 0) { // Nothing in Sample Buffer
						if (!ch.length_counter && ch.reg.loop_enabled) {
							ch.length_counter = ch.reg.pcm_length * 16 + 1;
							ch.address = (ch.reg.reg0 | 0x300) << 6;
						}
						if (ch.length_counter > 0) {
							// NOTE: Re-entrant! But not recursive, because even the shortest wavelength is greater than read time.
							// TODO: Proper clock

							// $8000 -> cartridge/gamepak
							if (ch.reg.wave_length > 20) for (unsigned t{ 0 }; t < 3;++t) // CPU::RB(u16(ch.address) | 0x8000) // Timing
								ch.hold = _cartridge_for_apu->cpu_read((ch.address++) | 0x8000); // Fetch Byte

							ch.phase = 8;
							--ch.length_counter;
						}
						else { // otherwise disable channel or issue IRQ
							apu->channels_enable[4] = ch.reg.irq_enable && (apu->dmc_irq = apu->irq_call = true);
						}

					}
					if (ch.phase != 0) { // Update the signal if sample buffered normally 
						int v = ch.linear_counter;
						if (ch.hold & (0x80 >> --ch.phase)) v += 2;
						else v -= 2;

						if (v >= 0 && v <= 0x7F) ch.linear_counter = v;
					}

					return S = ch.linear_counter;
				}
			}

		} channels[5] = {};

		struct { short lo, hi; } hz240counter = { 0,0 };

		// idx -> APU Address
		void write(u8 idx, u8 value) {
			channel& ch = channels[(idx / 4) % 5]; // % 5 is used to limit the range to [0,4] | idx is address, here used to calculate the channel no.
			switch (idx < 0x10 ? idx % 4 : idx) { // % 4 limits the range to [0,3]
			case 0:
				if (ch.reg.linear_counter_disable) ch.linear_counter = value & 0x7F;
				ch.reg.reg0 = value;
				break;

			case 1:
				ch.reg.reg1 = value;
				ch.sweep_delay = ch.reg.sweep_rate;
				break;

			case 2: ch.reg.reg2 = value; break;

			case 3:
				ch.reg.reg3 = value;
				if (channels_enable[idx / 4]) {
					ch.length_counter = length_counters[ch.reg.length_counter_init];
				}
				ch.linear_counter = ch.reg.linear_counter_init;
				ch.env_delay = ch.reg.env_decay_rate;
				ch.envelope = 15;
				if (idx < 8) ch.phase = 0;
				break;

			case 0x10:
				ch.reg.reg3 = value;
				ch.reg.wave_length = dmc_periods[value & 0x0F];
				break;

			case 0x11:
				ch.linear_counter = value & 0x7F; // DAC value
				break;

			case 0x12:
				ch.reg.reg0 = value;
				ch.address = (ch.reg.reg0 | 0x300) << 6;
				break;

			case 0x13:
				ch.reg.reg1 = value;
				ch.length_counter = ch.reg.pcm_length * 16 + 1;
				break;

			case 0x15:
				for (unsigned c = 0; c < 5;++c)
					channels_enable[c] = value & (1 << c);
				for (unsigned c = 0; c < 5;++c) {
					if (!channels_enable[c])
						channels[c].length_counter = 0;
					else if (c == 4 && channels[c].length_counter == 0)
						channels[c].length_counter = ch.reg.pcm_length * 16 + 1;
				}
				break;

			case 0x17:
				irq_disable = value & 0x40;
				five_cycle_divider = value & 0x80;
				hz240counter = { 0,0 };
				if (irq_disable) periodic_irq = dmc_irq = false;
				break;

			default: break;
			}
		}

		u8 read() {
			u8 res = 0;
			for (unsigned c = 0; c < 5; ++c)
				res |= (channels[c].length_counter ? 1 << c : 0);
			if (periodic_irq)
				res |= 0x40;
			periodic_irq = false;
			if (dmc_irq)
				res |= 0x80;
			dmc_irq = false;
			irq_call = false;
			return res;
		}

		void clock() {
			// divide cpu clock by 7457.5 to get a 240hz, which controls certain events
			if ((hz240counter.lo += 2) >= 14915) {
				hz240counter.lo -= 14915;
				if (++hz240counter.hi >= 4 + five_cycle_divider) hz240counter.hi = 0;

				// 60 Hz interval: irq | IRQ is not invoked in five-cycle mode (48Hz)
				if (!irq_disable && !five_cycle_divider && hz240counter.hi == 0) {
					periodic_irq = irq_call = true;
				}

				// Some events are invoked at 96Hz or 120Hz rate. Others at 192Hz or 240Hz.
				bool half_clock = (hz240counter.hi & 5) == 1, full_clock = hz240counter.hi < 4;

				for (unsigned c{ 0 }; c < 4;++c) {
					channel& ch = channels[c];
					int wl = ch.reg.wave_length;

					// Length Tick (All channels except DMC, but different disable bit for TRI ch)
					if (half_clock && ch.length_counter && !(c == 2 ? ch.reg.linear_counter_disable : ch.reg.length_counter_disable)) ch.length_counter -= 1; // Decrement

					// Sweep Tick (SQR ch only)
					if (half_clock && c < 2 && count(ch.sweep_delay, ch.reg.sweep_rate)) {
						if (wl >= 8 && ch.reg.sweep_enable && ch.reg.sweep_shift) {
							int s = wl >> ch.reg.sweep_shift, d[4] = { s,s,~s,-s };
							wl += d[ch.reg.sweep_decrease * 2 + c];
							if (wl < 0x800) ch.reg.wave_length = wl;
						}
					}

					// Linear Tick (TRI ch only)
					if (full_clock && c == 2) {
						ch.linear_counter = ch.reg.linear_counter_disable ? ch.reg.linear_counter_init : (ch.linear_counter > 0 ? ch.linear_counter - 1 : 0);
					}

					// Envelope Tick (SQR and Noise ch only)
					if (full_clock && c != 2 && count(ch.env_delay, ch.reg.env_decay_rate)) {
						if (ch.envelope > 0 || ch.reg.env_decay_loop_enable)
							ch.envelope = (ch.envelope - 1) & 15;
					}
				}
			}
		}

			bool irq_call{ false };
		private:

			bool five_cycle_divider = false, irq_disable = false;
			bool channels_enable[5] = { false };
			bool periodic_irq = false, dmc_irq = false;
			bool count(int& v, int reset) {
				return --v < 0 ? (v = reset), true : false;
			}
	};

} // namespace APU