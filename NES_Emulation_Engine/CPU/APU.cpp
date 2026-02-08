#include "APU.h"

#if AUDIO_TEST
#define _USE_MATH_DEFINES
#include <math.h> // for sin()
#endif // AUDIO_TEST

namespace NES::Audio {
	void APU::clock() {
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

			for (unsigned c{ 0 }; c < 4; ++c) {
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
#if AUDIO_TEST
			float amplitude = (float)sin(playbackTime * 2 * M_PI * TONE_HZ);
			int16_t y = (int16_t)(TONE_VOLUME * amplitude);

			output_buffer[sample_count++] = y;
			//*buffer++ = y;

			playbackTime += 1.f / 48000;
#else
			if (sample_count < max_buffer_size) {
				output_buffer[sample_count++] = 0;
			}
#endif
		}
	}
}