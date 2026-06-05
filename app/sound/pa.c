/* pulse-simple-play.c:
 *
 * Example of using the Pulseaudio simple API to play a raw sample
 * embedded within the executable.  
 *
 * Compile:
 *   cc -Wall -O2 -lpulse-simple pulse-simple-play.c
 */

#include <pulse/simple.h>

/* Create the pcm_raw.h as follows:
 *
 *   sox sample.wav -t raw -e signed -b 16 -L -r 44100 -c 2 >pcm.raw
 *   xxd -i pcm.raw > pcm_raw.h
 *
 */
#include "pcm_raw.h"


int main(int argc, char *argv[])
{
        int error;
        pa_simple *s = NULL;
        static const pa_sample_spec ss = {
                .format = PA_SAMPLE_S16LE,
                .rate = 44100,
                .channels = 2
        };

        s = pa_simple_new(NULL, argv[0], PA_STREAM_PLAYBACK, NULL, "playback", &ss, NULL, NULL, &error);
        if (s) {
                pa_simple_write(s, pcm_raw, pcm_raw_len, &error);
                pa_simple_drain(s, &error);
                pa_simple_free(s);
        }

        return 0;
}

