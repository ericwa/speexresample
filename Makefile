CFLAGS=$(shell pkg-config --cflags speex sndfile)
LIBS=$(shell pkg-config --libs speex sndfile)

speexresample: speexresample.c
	$(CC) $^ $(CFLAGS) $(LIBS) -o speexresample
