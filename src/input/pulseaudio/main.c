#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pulse/simple.h>
#include <pulse/error.h>
#include <pulse/pulseaudio.h>
#include <pthread.h>

#include "../../shared.h"

pa_mainloop *m_pulseaudio_mainloop;

void cb(__attribute__((unused)) pa_context *pulseaudio_context, const pa_server_info *i, void *userdata){
	//getting default sink name
	struct audio_data *audio = (struct audio_data *)userdata;
	audio->source = malloc(sizeof(char) * 1024);

	strcpy(audio->source,i->default_sink_name);

	//appending .monitor suufix
	audio->source = strcat(audio->source, ".monitor");

	//quiting mainloop
	pa_context_disconnect(pulseaudio_context);
	pa_context_unref(pulseaudio_context);
	pa_mainloop_quit(m_pulseaudio_mainloop, 0);
	pa_mainloop_free(m_pulseaudio_mainloop);
}


void pulseaudio_context_state_callback(pa_context *pulseaudio_context,
														void *userdata) {
	//make sure loop is ready
	switch (pa_context_get_state(pulseaudio_context)) {
		case PA_CONTEXT_UNCONNECTED:
			xavaSpam("UNCONNECTED");
			break;
		case PA_CONTEXT_CONNECTING:
			xavaSpam("CONNECTING");
			break;
		case PA_CONTEXT_AUTHORIZING:
			xavaSpam("AUTHORIZING");
			break;
		case PA_CONTEXT_SETTING_NAME:
			xavaSpam("SETTING_NAME");
			break;
		case PA_CONTEXT_READY://extract default sink name
			xavaSpam("READY");
			pa_operation_unref(pa_context_get_server_info(
			pulseaudio_context, cb, userdata));
			break;
		case PA_CONTEXT_FAILED:
			xavaBail("Fail to connect to pulseaudio server");
			break;
		case PA_CONTEXT_TERMINATED:
			xavaSpam("TERMINATED");
			pa_mainloop_quit(m_pulseaudio_mainloop, 0);
			break;
	}
}


void getPulseDefaultSink(void* data) {
	struct audio_data *audio = (struct audio_data *)data;
	pa_mainloop_api *mainloop_api;
	pa_context *pulseaudio_context;
	int ret;

	// Create a mainloop API and connection to the default server
	m_pulseaudio_mainloop = pa_mainloop_new();

	mainloop_api = pa_mainloop_get_api(m_pulseaudio_mainloop);
	pulseaudio_context = pa_context_new(mainloop_api, "xava device list");

	// This function connects to the pulse server
	pa_context_connect(pulseaudio_context, NULL, PA_CONTEXT_NOFLAGS, NULL);

	xavaSpam("Connecting to PulseAudio server");

	// This function defines a callback so the server will tell us its state.
	pa_context_set_state_callback(pulseaudio_context,
		pulseaudio_context_state_callback, (void*)audio);

	// starting a mainloop to get default sink

	// starting with one nonblokng iteration in case pulseaudio is not able to run
	xavaBailCondition(!(ret=pa_mainloop_iterate(m_pulseaudio_mainloop, 0, &ret)),
			"Could not open PulseAudio mainloop to find device name: %d\n"
			"Check if PulseAudio is running!\n", ret);

	pa_mainloop_run(m_pulseaudio_mainloop, &ret);
}

EXP_FUNC void* xavaInput(void* data)
{
	struct audio_data *audio = (struct audio_data *)data;
	int i, n;
	int16_t buf[audio->inputsize];

	// get default audio source if there is none
	if(strcmp(audio->source, "auto") == 0) {
		getPulseDefaultSink((void*)audio);
	}

	/* The sample type to use */
	static const pa_sample_spec ss = {
		.format = PA_SAMPLE_S16LE,
		.rate =  44100,
		.channels = 2
		};
	static pa_buffer_attr pb = {
		.maxlength = (uint32_t) -1, //BUFSIZE * 2,
		.fragsize = 0
	};

	// the following code reduces the time needed for a buffer update,
	// so you can have a high audio input size whilst the buffer update
	// still being recent enough
	// tl;dr: reduces delay, don't touch
	pb.fragsize = audio->inputsize > 1024 ? 1024 : audio->inputsize;

	pa_simple *s = NULL;
	int error;

	if (!(s = pa_simple_new(NULL, "XAVA", PA_STREAM_RECORD, audio->source, "XAVA audio input", &ss, NULL, &pb, &error))) {
		sprintf(audio->error_message, __FILE__": Could not open pulseaudio source: %s, %s. To find a list of your pulseaudio sources run 'pacmd list-sources'\n", audio->source, pa_strerror(error));
		audio->terminate = 1;
		pthread_exit(NULL);
	}

	n = 0;

	while (1) {
		/* Record some data ... */
		if (pa_simple_read(s, buf, sizeof(buf), &error) < 0) {
			//fprintf(stderr, __FILE__": pa_simple_read() failed: %s\n", pa_strerror(error));
			//exit(EXIT_FAILURE);
			sprintf(audio->error_message, __FILE__": pa_simple_read() failed: %s\n", pa_strerror(error));
			audio->terminate = 1;
			pthread_exit(NULL);
		}

		//sorting out channels
		for (i = 0; i < audio->inputsize; i += 2) {
			if (audio->channels == 1) audio->audio_out_l[n] = (buf[i] + buf[i + 1]) / 2;

			//stereo storing channels in buffer
			if (audio->channels == 2) {
				audio->audio_out_l[n] = buf[i];
				audio->audio_out_r[n] = buf[i + 1];
			}

			n++;
			if (n == audio->inputsize) n = 0;
		}
		if (audio->terminate == 1) {
			pa_simple_free(s);
			free(audio->source);
			break;
		}
	}

	return 0;
}

EXP_FUNC void xavaInputHandleConfiguration(struct XAVA_HANDLE *xava) {
	struct audio_data *audio = &xava->audio;
	XAVACONFIG config = xava->default_config.config;
	audio->rate = 44100;
	audio->source = (char*)xavaConfigGetString(config, "input", "source", "auto");
}

