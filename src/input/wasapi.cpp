#include <cstdio>
#include <ctime>
#include <windows.h>
#include <mmsystem.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <avrt.h>
#include <functiondiscoverykeys_devpkey.h>
#include "fifo.h"
#include "wasapi.h"

using namespace std;

#define BUFSIZE 4096

const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient = __uuidof(IAudioClient);
const IID IID_IAudioCaptureClient = __uuidof(IAudioCaptureClient);

static struct audio_data *audio;
static int n;

HRESULT sinkSetFormat(WAVEFORMATEX * pWF)
{
	// For the time being, just return OK.
	pWF->wFormatTag = WAVE_FORMAT_PCM;
	pWF->nChannels = audio->channels;
	pWF->nSamplesPerSec = 44100;
	pWF->nAvgBytesPerSec = 88200*audio->channels;
	pWF->nBlockAlign = 2*audio->channels;
	pWF->wBitsPerSample = 16;
	return ( S_OK ) ;
}

HRESULT sinkCopyData(BYTE * pData, UINT32 NumFrames)
{
	int *pBuffer = (int*)pData;
	for(UINT32 i=0; i<NumFrames; i++) {
		
		// convert binary offset to two's complement
		if(pBuffer[0]&0x80000000) {
			pBuffer[0]^=0x7FFFFFFF;
			pBuffer[0]++;
		}
		if(pBuffer[1]&0x80000000) {
			pBuffer[1]^=0x7FFFFFFF;
			pBuffer[1]++;
		}
		
		switch(audio->channels) {
			case 1:
				audio->audio_out_l[n] = (*pBuffer++ + *pBuffer++) >> 17;
				break;
			case 2:
				audio->audio_out_l[n] = *pBuffer++ >> 16;
				audio->audio_out_r[n] = *pBuffer++ >> 16;
				break;
		}
		n++;
		if(n == 2048-1) n = 0; // tfw fucking bugs make software better
	}
	return S_OK ;
}

//-----------------------------------------------------------
// Record an audio stream from the default audio capture
// device. The RecordAudioStream function allocates a shared
// buffer big enough to hold one second of PCM audio data.
// The function uses this buffer to stream data from the
// capture device. The main loop runs every 1/2 second.
//-----------------------------------------------------------

// REFERENCE_TIME time units per second and per millisecond
#define REFTIMES_PER_SEC  10000000
#define REFTIMES_PER_MILLISEC  10000

void* input_wasapi(void *audiodata)
{
	HRESULT hr;
	REFERENCE_TIME hnsRequestedDuration = REFTIMES_PER_SEC;
	UINT32 bufferFrameCount;
	UINT32 numFramesAvailable;
	IMMDeviceEnumerator *pEnumerator = NULL;
	IMMDevice *pDevice = NULL;
	IAudioClient *pAudioClient = NULL;
	IAudioCaptureClient *pCaptureClient = NULL;
	WAVEFORMATEX *pwfx = NULL;
	UINT32 packetLength = 0;
	BYTE *pData;
	DWORD flags;
	audio = (struct audio_data *)audiodata;
	REFERENCE_TIME hnsDefaultDevicePeriod;

	n = 0; // reset the buffer counter

	hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);

	hr = CoCreateInstance(
			CLSID_MMDeviceEnumerator, NULL,
			CLSCTX_ALL, IID_IMMDeviceEnumerator,
			(void**)&pEnumerator);
	if(hr) {
		MessageBox(NULL, "CoCreateInstance failed - Couldn't get IMMDeviceEnumerator", "Error", MB_OK | MB_ICONERROR);
		exit(EXIT_FAILURE);
	}

	hr = pEnumerator->GetDefaultAudioEndpoint(
			strcmp(audio->source, "loopback") ? eCapture : eRender, eConsole, &pDevice);
	if(hr) {
		MessageBox(NULL, "Failed to get default audio endpoint", "Error", MB_OK | MB_ICONERROR);
		exit(EXIT_FAILURE);
	}

	hr = pDevice->Activate(
		IID_IAudioClient, CLSCTX_ALL,
		NULL, (void**)&pAudioClient);
	if(hr) {
		MessageBox(NULL, "Failed setting up default audio endpoint", "Error", MB_OK | MB_ICONERROR);
		exit(EXIT_FAILURE);
	}

	hr = pAudioClient->GetMixFormat(&pwfx);
	if(hr) {
		MessageBox(NULL, "Failed getting default audio endpoint mix format", "Error", MB_OK | MB_ICONERROR);
		exit(EXIT_FAILURE);
	}

	hr = pAudioClient->Initialize(
			AUDCLNT_SHAREMODE_SHARED,
			strcmp(audio->source, "loopback") ? 0 : AUDCLNT_STREAMFLAGS_LOOPBACK,
			hnsRequestedDuration,
			0,
			pwfx,
			NULL);
	if(hr) {
		MessageBox(NULL, "Failed initilizing default audio endpoint", "Error", MB_OK | MB_ICONERROR);
		exit(EXIT_FAILURE);
	}

	// Get the size of the allocated buffer.
	hr = pAudioClient->GetBufferSize(&bufferFrameCount);
	if(hr) {
		MessageBox(NULL, "Failed getting buffer size", "Error", MB_OK | MB_ICONERROR);
		exit(EXIT_FAILURE);
	}

	hr = pAudioClient->GetService(
		IID_IAudioCaptureClient,
		(void**)&pCaptureClient);
	if(hr) {
		MessageBox(NULL, "Failed getting capture service", "Error", MB_OK | MB_ICONERROR);
		exit(EXIT_FAILURE);
	}

	// Set the audio sink format to use.
	hr = sinkSetFormat(pwfx);
	if(hr) {
		MessageBox(NULL, "Failed setting format", "Error", MB_OK | MB_ICONERROR);
		exit(EXIT_FAILURE);
	}

	hr = pAudioClient->Start();  // Start recording.
	if(hr) {
		MessageBox(NULL, "Failed starting capture", "Error", MB_OK | MB_ICONERROR);
		exit(EXIT_FAILURE);
	}
	
	hr = pAudioClient->GetDevicePeriod(&hnsDefaultDevicePeriod, NULL);
	if(hr) {
		MessageBox(NULL, "Error getting device period", "Error", MB_OK | MB_ICONERROR);
		exit(EXIT_FAILURE);
	}

	LONG lTimeBetweenFires = (LONG)hnsDefaultDevicePeriod / 2 / (10 * 1000); // convert to milliseconds

	// Each loop fills about half of the shared buffer.
	while (!audio->terminate)
	{
		Sleep(lTimeBetweenFires);

		hr = pCaptureClient->GetNextPacketSize(&packetLength);
		if(hr) {
			MessageBox(NULL, "Failure getting buffer size", "Error", MB_OK | MB_ICONERROR);
			exit(EXIT_FAILURE);
		}

		while (packetLength != 0)
		{
			// Get the available data in the shared buffer.
			hr = pCaptureClient->GetBuffer(
					&pData,
					&numFramesAvailable,
					&flags, NULL, NULL);
			if(hr) {
				MessageBox(NULL, "Failure to capture available buffer data", "Error", MB_OK | MB_ICONERROR);
				exit(EXIT_FAILURE);
			}

			//if (flags & AUDCLNT_BUFFERFLAGS_SILENT) pData = NULL;  // Tell CopyData to write silence.

			// Copy the available capture data to the audio sink.
			hr = sinkCopyData(pData, numFramesAvailable);
			if(hr) {
				MessageBox(NULL, "Failure copying buffer data", "Error", MB_OK | MB_ICONERROR);
				exit(EXIT_FAILURE);
			}

			hr = pCaptureClient->ReleaseBuffer(numFramesAvailable);
			if(hr) {
				MessageBox(NULL, "Failed to release buffer", "Error", MB_OK | MB_ICONERROR);
				exit(EXIT_FAILURE);
			}

			hr = pCaptureClient->GetNextPacketSize(&packetLength);
			if(hr) {
				MessageBox(NULL, "Failure getting buffer size", "Error", MB_OK | MB_ICONERROR);
				exit(EXIT_FAILURE);
			}
		}
	}

	hr = pAudioClient->Stop();  // Stop recording.
	if(hr) {
		MessageBox(NULL, "Failure stopping capture", "Error", MB_OK | MB_ICONERROR);
		exit(EXIT_FAILURE);
	}

	CoTaskMemFree(pwfx);
	pEnumerator->Release();
	pDevice->Release();
	pAudioClient->Release();
	pCaptureClient->Release();

	CoUninitialize();

	return 0;
}

