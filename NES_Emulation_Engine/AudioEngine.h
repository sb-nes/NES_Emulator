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

	AudioEngine(NES::CPU::R6502& r6502) : _R6502(r6502) {}
	
private:
#ifdef WINDOWS_GDI
	IAudioClient2*			_audioClient;
	IAudioRenderClient*		_audioRenderClient;

	UINT32					_bufferSizeInFrames;

	void queue_audio();
#else
	SDL_AudioStream*		_stream { NULL };
	SDL_AudioDeviceID		_device;
#endif
	int						_avg_queue_size;
	NES::CPU::R6502&		_R6502;
};