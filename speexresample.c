#include <stdio.h>
#include <stdlib.h>

#include <speex/speex_resampler.h>
#include <sndfile.h>

#define BUFFERSAMPLES 256

int speexresample(const char *infile, const char *outfile, const int outrate, const int quality)
{
	printf("infile: %s outfile: %s outrate: %d quality: %d\n", infile, outfile, outrate, quality); 

	SF_INFO infile_info = {0};
	SNDFILE *infile_sndfile = sf_open(infile, SFM_READ, &infile_info);

	SF_INFO outfile_info = { 
		.frames = 0,
		.samplerate = outrate,
		.channels = infile_info.channels,
		.format = infile_info.format,
		.sections = 0,
		.seekable = 0
	};
	SNDFILE *outfile_sndfile = sf_open(outfile, SFM_WRITE, &outfile_info);

	
	int resampler_err = 0;
	SpeexResamplerState *resampler_state = 
		speex_resampler_init(infile_info.channels,
							 infile_info.samplerate,
							 outrate,
							 quality,
							 &resampler_err);

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
		}
	}

	speex_resampler_destroy(resampler_state);

	sf_close(infile_sndfile);
	sf_close(outfile_sndfile);

	return 0;
}

int main(int argc, const char **argv)
{
	if (argc != 5)
	{
		printf("usage: speexresample infile.wav outfile.wav <output-samplerate> <quality>\n");
		printf("\tquality: 0-10, where 0 is worst, 10 is best\n");
		return 0;
	}

	return speexresample(argv[1], argv[2], atoi(argv[3]), atoi(argv[4]));
}
