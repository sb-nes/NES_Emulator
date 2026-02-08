#include "AudioEngine.h"

#include "Common/CommonHeaders.h"
#include <cstdio> // For Legacy Code

#if AUDIO_TEST_B or AUDIO_TEST_C
#define _USE_MATH_DEFINES
#include <math.h> // for sin()
#endif // AUDIO_TEST_B


namespace { 
	// Using constexpr rather than #define, so that we can calculate once and store it for later use, rather than
	// just replace the code to re-calculate everytime I need a value. [Predetermined value]
	constexpr int			sample_rate = 48000; // Nominal Frequency
	constexpr float			max_frequency_difference = 0.02f;
	constexpr int			target_queue_size = sample_rate / 60 * 3; // for 48kHz sample rate, it is 2400.

	constexpr float			target_buffer_padding = 1 / 50.f;
	// How much padding we want our sound buffer to have after writing to it. Needs to be enough so that the playback doesn't reach garbage data
	// but we get less latency the lower it is (i.e. how long does it take between pressing jump and hearing the sound effect)
	// Try setting this to e.g. 1/250.f to hear what happens when we're not writing enough data to stay ahead of playback!

} // Anonymous Namespace

bool AudioEngine::initialize_engine(){
	// 1. Initialize Audio for Window Handler [for SDL]
	// 2. Set the Specifications/Format for audio device
	// 3. Initialize Audio Device and Open it
#ifdef WINDOWS_GDI
	// I found examples of WASAPI from github user [kevinmoran]
	HRESULT	hr;
	WAVEFORMATEX format_specification = { .wFormatTag = WAVE_FORMAT_PCM, //WAVE_FORMAT_48S16
										  .nChannels = 1,
										  .nSamplesPerSec = sample_rate,
										  .wBitsPerSample = 16,
										  .cbSize = 0, }; // cbSize -> Extra Information for the format
	
	format_specification.nBlockAlign = (format_specification.nChannels * format_specification.wBitsPerSample) / 8;
	format_specification.nAvgBytesPerSec = format_specification.nSamplesPerSec * format_specification.nBlockAlign;

	// Get Audio Device Client
	{	// To get an audio device to use (usually the default one), we use a device enumerator.
		IMMDevice* audioDevice;
		hr = CoInitializeEx(nullptr, COINIT_SPEED_OVER_MEMORY); // ???
		assert(hr == S_OK);

		IMMDeviceEnumerator* _deviceEnumerator;
		hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (LPVOID*)(&_deviceEnumerator));
		assert(hr == S_OK);
		hr = _deviceEnumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &audioDevice); // Get the default audio device
		assert(hr == S_OK);
		_deviceEnumerator->Release(); // Release when an audio device is found.
		
		hr = audioDevice->Activate(__uuidof(IAudioClient2), CLSCTX_ALL, nullptr, (LPVOID*)(&_audioClient));
		assert(hr == S_OK);
		audioDevice->Release();
	}


	// TODO: Fix the Maximum Buffer Size
	const s64 REFTIMES_PER_SEC = 10000000; // hundred nanoseconds
	REFERENCE_TIME maxBufferDuration = (REFERENCE_TIME)(REFTIMES_PER_SEC * 2); // in nanoseconds, rather than blocks/frames

	DWORD initStreamFlags = (AUDCLNT_STREAMFLAGS_RATEADJUST | AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM | AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY);
	hr = _audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, initStreamFlags, maxBufferDuration, 0, &format_specification, nullptr);
	assert(hr == S_OK);

	hr = _audioClient->GetService(__uuidof(IAudioRenderClient), (LPVOID*)(&_audioRenderClient));
	assert(hr == S_OK);
	hr = _audioClient->GetBufferSize(&_bufferSizeInFrames);
	assert(hr == S_OK);
	hr = _audioClient->Start();
	assert(hr == S_OK);

	return true;
#else
	SDL_AudioSpec spec;

	if (SDL_Init(SDL_INIT_AUDIO) < 0) {
		SDL_Log("SDL_INIT_AUDIO failed: %s", SDL_GetError());
		//fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
		return false;
	}

	SDL_zero(spec);
	spec.channels = 1;
	// spec.samples = 1024; Block Size Removed
	// spec.callback = NULL; Decrepit
	// spec.format = AUDIO_S16SYS;
	spec.format = SDL_AUDIO_S16;
	spec.freq = sample_rate;

	_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
	if (!_stream) {
		SDL_Log("Couldn't create audio stream: %s", SDL_GetError());
		return false;
	}

	SDL_ResumeAudioStreamDevice(_stream); // SDL_OpenAudioDeviceStream starts the device paused. You have to tell it to start!
	
	if ((_device = SDL_OpenAudioDevice(0, &spec)) < 0) {
		SDL_Log("SDL_OpenAudioDevice failed: %s\n", SDL_GetError());
		// fprintf(stderr, "SDL_OpenAudioDevice failed: %s\n", SDL_GetError());
		return false;
	}
	SDL_PauseAudioDevice(_device);

	return true;
#endif
}

void AudioEngine::destroy_engine(){
#ifdef WINDOWS_GDI
	_audioClient->Stop();
	_audioClient->Release();
	_audioRenderClient->Release();
#else
	SDL_CloseAudioDevice(_device);
#endif // WINDOWS_GDI
}

#ifdef WINDOWS_GDI
void AudioEngine::queue_audio(UINT32 available_frames) {
	HRESULT hr;
	s16* buffer;
	UINT32 numFramesToWrite = _bus._apu.sample_count;

#if AUDIO_TEST_B
	hr = _audioRenderClient->GetBuffer(available_frames, (BYTE**)(&buffer));
	assert(hr == S_OK);

	for (UINT32 frameIndex = 0; frameIndex < available_frames; ++frameIndex) {
		float amplitude = (float)sin(playbackTime * 2 * M_PI * TONE_HZ);
		int16_t y = (int16_t)(TONE_VOLUME * amplitude);

		*buffer++ = y; // left

		playbackTime += 1.f / sample_rate;
	}

	hr = _audioRenderClient->ReleaseBuffer(available_frames, 0);
	assert(hr == S_OK);
#elif AUDIO_TEST_C
	hr = _audioRenderClient->GetBuffer(numFramesToWrite, (BYTE**)(&buffer));
	assert(hr == S_OK);
	int16_t y = 0;
	for (UINT32 frameIndex = 0; frameIndex < numFramesToWrite; ++frameIndex) {
		float amplitude = (float)sin(playbackTime * 2 * M_PI * TONE_HZ);
		y = (int16_t)(TONE_VOLUME * amplitude);

		*buffer++ = y; // left

		playbackTime += 1.f / 5000;
	}
	_last_know_state = y;

	hr = _audioRenderClient->ReleaseBuffer(numFramesToWrite, 0);
	assert(hr == S_OK);
#else
	// Assign Buffer's address into *buffer
	hr = _audioRenderClient->GetBuffer(numFramesToWrite, (BYTE**)(&buffer));
	assert(hr == S_OK);
	// Fill Buffer
	for (UINT32 i = 0; i < numFramesToWrite; ++i) {
		*buffer++ = _bus._apu.output_buffer[i]; // Left
	}
	// Release Buffer
	hr = _audioRenderClient->ReleaseBuffer(numFramesToWrite, 0);
	assert(hr == S_OK);
#endif // AUDIO_TEST_C

	
}
#endif

void AudioEngine::output_audio(){
	// 1. Get Current Audio Queue Size
	// 2. [Optional] Get the moving average queue size
	// 3. [Optional] Adjust sample frequency as an attempt to maintain a constant queue size
	// 4. Check if queue is completely filled to target queue size
	// 5. if yes, Skip Audio Frame
	// 6. else, queue audio buffer from APU's buffer
	// 7. clear APU's Buffer
#ifdef WINDOWS_GDI
	// Padding is how much valid data is queued up in the sound buffer
	// if there's enough padding then we could skip writing more data
	HRESULT hr;
	UINT32 bufferPadding; // how many are in the Queue?
	hr = _audioClient->GetCurrentPadding(&bufferPadding);
	assert(hr == S_OK);

	int wavPlaybackSample = 0;
	
	UINT32 targetBufferPadding = UINT32(_bufferSizeInFrames * target_buffer_padding);  // 96000 / 187.5 = 512 Frames or Block Size. Should be constant, so TODO: make it calculate once.
	UINT32 available_frames = targetBufferPadding - bufferPadding;

	if (bufferPadding > targetBufferPadding * 2) { // Skip Frame
	} else { 
		queue_audio(available_frames); 
	}

	// TODO: clear APU's buffer
	_bus._apu.clear_output();
#else
	// Exponential moving average of audio queue size
	int queue_size = SDL_GetQueuedAudioSize(audio_device) / sizeof(int16_t);
	constexpr float alpha = 0.1f;
	average_queue_size = (int)(queue_size * alpha + average_queue_size * (1.0f - alpha));

	// Adjust sample frequency to try and maintain a constant queue size
	float diff = (float)(average_queue_size - target_queue_size) / target_queue_size;
	diff = std::min(std::max(diff, -1.0f), 1.0f);

	int sample_rate = (int)(nominal_frequency * (1.0f - diff * max_frequency_difference));
	nes.apu.set_sample_rate(sample_rate);

	if (queue_size > target_queue_size * 2) { // Queue is too large, just skip this frame's audio to catch up faster 
	}
	else {
		SDL_QueueAudio(audio_device, nes.apu.output_buffer, nes.apu.sample_count * sizeof(int16_t));
	}
	nes.apu.clear_output_buffer();
#endif
}
