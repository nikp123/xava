#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <portaudio.h>

#include "main.h"
#include "../../shared.h"

#define SAMPLE_SILENCE 32768
#define PA_SAMPLE_TYPE paInt16
typedef short SAMPLE;

typedef struct {
	int          frameIndex;  /* Index into sample array. */
	int          maxFrameIndex;
	SAMPLE      *recordedSamples;
} paTestData;

static struct audio_data *audio;
static int n = 0;

static int recordCallback(const void *inputBuffer, void *outputBuffer,
	unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo,
	PaStreamCallbackFlags statusFlags, void *userData) {
	paTestData *data = (paTestData*)userData;
	const SAMPLE *rptr = (const SAMPLE*)inputBuffer;
	long framesToCalc;
	long i;
	int finished;
	unsigned long framesLeft = data->maxFrameIndex - data->frameIndex;

	(void) outputBuffer; // Prevent unused variable warnings.
	(void) timeInfo;
	(void) statusFlags;
	(void) userData;

	if( framesLeft < framesPerBuffer ) {
		framesToCalc = framesLeft;
		finished = paComplete;
	} else {
		framesToCalc = framesPerBuffer;
		finished = paContinue;
	}

	if(inputBuffer == NULL) {
		for(i=0; i<framesToCalc; i++) {
			if(audio->channels == 1) audio->audio_out_l[n] = SAMPLE_SILENCE;
			if(audio->channels == 2) {
				audio->audio_out_l[n] = SAMPLE_SILENCE;
				audio->audio_out_r[n] = SAMPLE_SILENCE;
			}
			if(n == audio->inputsize-1) n = 0;
		}
	} else {
		for(i=0; i<framesToCalc; i++) {
			if(audio->channels == 1) {
				audio->audio_out_l[n] = (rptr[0] + rptr[1]) / 2;
				rptr += 2;
			}
			if(audio->channels == 2) {
				audio->audio_out_l[n] = *rptr++;
				audio->audio_out_r[n] = *rptr++;
			}
			n++;
			if(n == audio->inputsize-1) n = 0;
		}
	}

	data->frameIndex += framesToCalc;
	if(finished == paComplete) {
		data->frameIndex = 0;
		finished = paContinue;
	}
	return finished;
}

EXP_FUNC void* xavaInput(void *audiodata) {
	audio = (struct audio_data *)audiodata;

	PaStreamParameters inputParameters;
	PaStream* stream;
	PaError err = paNoError;
	paTestData data;

	// start portaudio
	err = Pa_Initialize();
	xavaBailCondition(err != paNoError,
			"Unable to initialize portaudio: %s", Pa_GetErrorText(err));

	// get portaudio device
	int deviceNum = -1, numOfDevices = Pa_GetDeviceCount();
	if(!strcmp(audio->source, "list")) {
		xavaErrorCondition(numOfDevices<0, "PortAudio was not able to find any audio devices (code 0x%x)",
				numOfDevices);
		for(int i = 0; i < numOfDevices; i++) {
			const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo(i);
			printf("Device #%d: %s\n"
				"\tInput Channels: %d\n"
				"\tOutput Channels: %d\n"
				"\tDefault SampleRate: %lf\n",
				i+1, deviceInfo->name, deviceInfo->maxInputChannels,
				deviceInfo->maxOutputChannels, deviceInfo->defaultSampleRate);
		}
		exit(EXIT_SUCCESS);
	} else if(!strcmp(audio->source, "auto")) {
		deviceNum = Pa_GetDefaultInputDevice();

		xavaBailCondition(deviceNum == paNoDevice,
				"PortAudio was not able to find any input devices!");
	} else if(sscanf(audio->source,"%d", &deviceNum)) {
		xavaBailCondition(deviceNum>numOfDevices,
				"Invalid input device!");
		deviceNum--;
	} else {
		for(int i = 0; i < numOfDevices; i++) {
			const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo(i);
			if(!strcmp(audio->source, deviceInfo->name)) {
				deviceNum=i;
				break;
			} 
		}
		xavaBailCondition(deviceNum==-1,
				"No such device '%s'", audio->source);
	}
	inputParameters.device = deviceNum;

	// set parameters
	size_t audioLenght = audio->inputsize > 1024 ? 1024 : audio->inputsize;
	data.maxFrameIndex = audioLenght;
	data.recordedSamples = (SAMPLE *)malloc(2*audioLenght*sizeof(SAMPLE));

	xavaBailCondition(!data.recordedSamples,
			"Memory allocation error!");
	memset(data.recordedSamples, 0x00, 2*audioLenght);

	inputParameters.channelCount = 2;
	inputParameters.sampleFormat = PA_SAMPLE_TYPE;
	inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;
	inputParameters.hostApiSpecificStreamInfo = NULL;

	// set it to work
	err = Pa_OpenStream(&stream, &inputParameters, NULL, audio->rate, audioLenght,
		paClipOff, recordCallback, &data);

	xavaBailCondition(err!=paNoError, "Failure in opening stream (0x%x)", err);

	// main loop
	while(1){
		// start recording
		data.frameIndex = 0;
		err = Pa_StartStream(stream);
		xavaBailCondition(err!=paNoError, "Failure in starting stream (0x%x)", err);

		//  record
		while((err = Pa_IsStreamActive(stream)) == 1) {
			Pa_Sleep(5);
			if(audio->terminate == 1) break;
		}

		// check for errors
		xavaBailCondition(err<0, "Failure during audio recording (%x)", err);

		// check if it bailed
		if(audio->terminate == 1) break;
	}
	// close stream
	xavaBailCondition((err = Pa_CloseStream(stream)) != paNoError,
			"Failure in closing stream (0x%x)", err);

	Pa_Terminate();
	free(data.recordedSamples);
	return 0;
} 

EXP_FUNC void xavaInputHandleConfiguration(struct XAVA_HANDLE *xava) {
	struct audio_data *audio = &xava->audio;
	XAVACONFIG config = xava->default_config.config;
	audio->rate = 44100;
	audio->source = (char*)xavaConfigGetString(config, "input", "source", "auto");
}

