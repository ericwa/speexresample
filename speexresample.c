/*
Copyright (c) 2014 Eric Wasylishen

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <speex/speex_resampler.h>
#include <sndfile.h>

#define BUFFERSAMPLES 256
#define MIN(x, y) ((x)<(y)?(x):(y))

static float max_amplitude;

void check_max_amplitude(float *array, int len)
{
	for (int i=0; i<len; i++)
	{
		float sample_amplitude = fabs(array[i]);
		if (sample_amplitude > max_amplitude)
		{
			max_amplitude = sample_amplitude;
		}
	}
}

static inline void scale(float *array, int len, float factor)
{
	if (factor == 1) return;
	
	for (int i=0; i<len; i++)
	{
		array[i] *= factor;
	}
}

int speexresample(const char *infile, const char *outfile, const int outrate, const int quality, const float scalefactor, const int usefloat)
{
	printf("infile: %s outfile: %s outrate: %d quality: %d\n", infile, outfile, outrate, quality); 

	SF_INFO infile_info = {0};
	SNDFILE *infile_sndfile = sf_open(infile, SFM_READ, &infile_info);
	if (infile_sndfile == NULL)
	{
		fprintf(stderr, "couldn't open infile %s\n", infile);
		return 1;
	}
	
	SF_INFO outfile_info = { 
		.frames = 0,
		.samplerate = outrate,
		.channels = infile_info.channels,
		.format = SF_FORMAT_WAV | (usefloat ? SF_FORMAT_FLOAT : SF_FORMAT_PCM_16),
		.sections = 0,
		.seekable = 0
	};
	
	SNDFILE *outfile_sndfile = sf_open(outfile, SFM_WRITE, &outfile_info);
	if (outfile_sndfile == NULL)
	{
		fprintf(stderr, "couldn't open outfile %s\n", outfile);
		return 1;
	}
	
	int resampler_err = 0;
	SpeexResamplerState *resampler_state = 
		speex_resampler_init(infile_info.channels,
							 infile_info.samplerate,
							 outrate,
							 quality,
							 &resampler_err);

	speex_resampler_skip_zeros(resampler_state);

	const int expected_samples_written = (double)infile_info.frames * ((double)outrate/(double)infile_info.samplerate);
	int total_samples_written = 0;

	float buffer[BUFFERSAMPLES * infile_info.channels];
	int fill_starting_at = 0;
	
	while (1)
	{
		int buffersize = (int)sf_readf_float(infile_sndfile, buffer + (fill_starting_at * infile_info.channels), BUFFERSAMPLES - fill_starting_at);
		buffersize += fill_starting_at;
		
		if (buffersize == 0)
			break;

		float outbuffer[BUFFERSAMPLES * infile_info.channels];

		uint32_t in_processed = buffersize;
		uint32_t out_processed = BUFFERSAMPLES;
		speex_resampler_process_interleaved_float(resampler_state,
												buffer,
												&in_processed,
												outbuffer,
												&out_processed);

		check_max_amplitude(outbuffer, out_processed * infile_info.channels);
		scale(outbuffer, out_processed * infile_info.channels, scalefactor);
		sf_writef_float(outfile_sndfile, outbuffer, out_processed);

		if (in_processed < buffersize)
		{
			int unprocessed_input = buffersize - in_processed;
			memmove(buffer, buffer + (in_processed * infile_info.channels), unprocessed_input * infile_info.channels * sizeof(float));
			fill_starting_at = unprocessed_input;
		}
		else
		{
			fill_starting_at = 0;
		}
		
		total_samples_written += out_processed;
	}

	// Feed the resampler zeros
	while (total_samples_written < expected_samples_written)
	{
		const int needed = expected_samples_written - total_samples_written;
		
		float buffer[BUFFERSAMPLES * infile_info.channels];
		float outbuffer[BUFFERSAMPLES * infile_info.channels];

		memset(buffer, 0, sizeof(buffer));
		
		uint32_t in_processed = BUFFERSAMPLES;
		uint32_t out_processed = MIN(needed, BUFFERSAMPLES);
		speex_resampler_process_interleaved_float(resampler_state,
												  buffer,
												  &in_processed,
												  outbuffer,
												  &out_processed);

		check_max_amplitude(outbuffer, out_processed * infile_info.channels);
		scale(outbuffer, out_processed * infile_info.channels, scalefactor);
		sf_writef_float(outfile_sndfile, outbuffer, out_processed);
		
		total_samples_written += out_processed;
	}

	speex_resampler_destroy(resampler_state);

	sf_close(infile_sndfile);
	sf_close(outfile_sndfile);

	printf("read %d samples, wrote %d samples\n", (int)infile_info.frames, total_samples_written);

	return 0;
}

int main(int argc, const char **argv)
{
	if (argc != 5 && argc != 6)
	{
		printf("usage: speexresample infile.wav outfile.wav <output-samplerate> <quality> [float]\n");
		printf("\tquality: 0-10, where 0 is worst, 10 is best\n");
		return 1;
	}

	int usefloat = (argc == 6
					&& 0 == strcmp("float", argv[5]));
	
	int result = speexresample(argv[1], argv[2], atoi(argv[3]), atoi(argv[4]), 1.0f, usefloat);
	
	if (!usefloat && max_amplitude > 1)
	{
		const float scalefactor = 1.0f / max_amplitude;
		printf("clipping detected, scaling by %f\n", scalefactor);
		result = speexresample(argv[1], argv[2], atoi(argv[3]), atoi(argv[4]), scalefactor, usefloat);
	}
	
	return result;
}
