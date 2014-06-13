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

#include <speex/speex_resampler.h>
#include <sndfile.h>

#define BUFFERSAMPLES 256
#define MIN(x, y) ((x)<(y)?(x):(y))

int speexresample(const char *infile, const char *outfile, const int outrate, const int quality)
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
		.format = infile_info.format,
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

	short buffer[BUFFERSAMPLES * infile_info.channels];

	while (1)
	{
		int buffersize = (int)sf_readf_short(infile_sndfile, buffer, BUFFERSAMPLES);
		if (buffersize == 0)
			break;

		for (int bufferpos = 0; bufferpos < buffersize; )
		{
			short outbuffer[BUFFERSAMPLES * infile_info.channels];

			uint32_t in_processed = buffersize - bufferpos;
			uint32_t out_processed = BUFFERSAMPLES;
			speex_resampler_process_interleaved_int(resampler_state,
													buffer + (bufferpos * infile_info.channels),
													&in_processed,
													outbuffer,
													&out_processed);

			sf_writef_short(outfile_sndfile, outbuffer, out_processed);

			bufferpos += in_processed;
			total_samples_written += out_processed;
		}
	}

	// Feed the resampler zeros
	while (total_samples_written < expected_samples_written)
	{
		const int needed = expected_samples_written - total_samples_written;
		
		short buffer[BUFFERSAMPLES * infile_info.channels];
		short outbuffer[BUFFERSAMPLES * infile_info.channels];

		memset(buffer, 0, sizeof(buffer));
		
		uint32_t in_processed = BUFFERSAMPLES;
		uint32_t out_processed = MIN(needed, BUFFERSAMPLES);
		speex_resampler_process_interleaved_int(resampler_state,
												buffer,
												&in_processed,
												outbuffer,
												&out_processed);

		sf_writef_short(outfile_sndfile, outbuffer, out_processed);
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
	if (argc != 5)
	{
		printf("usage: speexresample infile.wav outfile.wav <output-samplerate> <quality>\n");
		printf("\tquality: 0-10, where 0 is worst, 10 is best\n");
		return 1;
	}

	return speexresample(argv[1], argv[2], atoi(argv[3]), atoi(argv[4]));
}
