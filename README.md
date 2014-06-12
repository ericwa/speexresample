speexresample
=============

by [Eric Wasylishen](mailto:ewasylishen@gmail.com)

Trivial wav file resampler using [libspeexdsp](http://www.speex.org) and [libsndfile](http://www.mega-nerd.com/libsndfile/). For more info on the speex resampler, see [the source code](http://svn.xiph.org/trunk/speex/libspeex/resample.c).

Build
-----

Should build on a Mac with just:

    brew install sndfile speex
    make
    
or any other platform with pkg-config.
    
Usage
-----

    ./speexresample infile.wav outfile.wav <output-samplerate> <quality>
    
quality: 0-10, where 0 is worst, 10 is best
