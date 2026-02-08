#pragma once

#include "GlobalSwitches.h"
#include "CPU/R6502.h"

#ifdef WINDOWS_GDI
// Using WASAPI
#include <mmdeviceapi.h>
#include <Audioclient.h>
#else
#include <SDL3/SDL.h>
#endif

using namespace NES::Audio;

class AudioEngine {

public:
	bool initialize_engine();
	void destroy_engine();
	void output_audio();

	AudioEngine(NES::CPU::R6502& r6502) : _R6502(r6502), _bus(r6502._bus) {}

	double playbackTime = 0.0;
	const float TONE_HZ = 440;
	const s16 TONE_VOLUME = 3000;
	
private:
#ifdef WINDOWS_GDI
	IAudioClient2*			_audioClient;
	IAudioRenderClient*		_audioRenderClient;

	UINT32					_bufferSizeInFrames;
	s16						_last_know_state{0};

	void queue_audio(UINT32 available_frames);
#else
	SDL_AudioStream*		_stream { NULL };
	SDL_AudioDeviceID		_device;
#endif
	int						_avg_queue_size;
	NES::CPU::R6502&		_R6502;
	NES::CPU::Bus&			_bus;
};