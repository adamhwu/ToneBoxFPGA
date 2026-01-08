#include <stdio.h>
#include <stdlib.h>
#include <portaudio.h>
#include <sndfile.h>  // For writing WAV files

#define NUM_CHANNELS 1
#define SAMPLE_FORMAT paInt16

typedef short SAMPLE; // 16-bit audio