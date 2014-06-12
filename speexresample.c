#include <stdio.h>
#include <stdlib.h>

int main(int argc, const char **argv)
{
	if (argc != 4)
	{
		printf("usage: speexresample infile.wav outfile.wav <output-samplerate>\n");
		return 0;
	}

	const char *infile = argv[1];
	const char *outfile = argv[2];
	const int outrate = atoi(argv[3]);

	printf("%s %s %d\n", infile, outfile, outrate); 

	return 0;
}
