CFLAGS=$(shell pkg-config --cflags speexdsp sndfile)
LIBS=$(shell pkg-config --libs speexdsp sndfile)

speexresample: speexresample.c
	$(CC) $^ $(CFLAGS) $(LIBS) -o speexresample
