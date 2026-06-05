/*
 * Simple sound playback using ALSA API and libasound.
 *
 * Compile:
 * $ gcc simple_playback.c -o simple_playback -lasound
 *
*/

#define ALSA_PCM_NEW_HW_PARAMS_API

#include <alsa/asoundlib.h>
#include <stdint.h>

static int periods_per_buffer = 1;// Down to user preference, depending on size of internal ring buffer of ALSA
#define FRAMES (1024*8)

// Structure to hold the metadata for writing wav files (to store the output)
typedef struct WaveHeader 
{
  /* RIFF Chunk Descriptor */
	uint8_t         RIFF[4];          // RIFF Header Magic header
	uint32_t        chunk_size;       // RIFF Chunk Size
	uint8_t         WAVE[4];          // WAVE Header
	/* "fmt" sub-chunk */
	uint8_t         fmt[4];           // FMT header
	uint32_t        subchunk1size;    // Size of the fmt chunk
	uint16_t        audio_format;     // Audio format 1=PCM,6=mulaw,7=alaw,
                                    // 257=IBM Mu-Law, 258=IBM A-Law, 259=ADPCM
	uint16_t        number_of_channels;      // Number of channels 1=Mono 2=Sterio
	uint32_t        samples_per_sec;  // Sampling Frequency in Hz
	uint32_t        bytes_per_sec;    // bytes per second
	uint16_t        block_align;      // 2=16-bit mono, 4=16-bit stereo
	uint16_t        bits_per_sample;  // Number of bits per sample
	/* "data" sub-chunk */
	uint8_t         subchunk2id[4];   // "data"  string
	uint32_t        subchunk2size;    // Sampled data length
} wav_header;

// Debug function to view contents of some wav header structure
static void * print_wav_header(wav_header * hdr) {
	char * str;
	printf("--- WAV HEADER ---\n");
	str = hdr->RIFF;
	printf("RIFF marker  : %c%c%c%c\n", str[0],str[1],str[2],str[3]);
	printf("File size    : %d\n", hdr->chunk_size);
	str = hdr->WAVE;	
	printf("File type    : %c%c%c%c\n", str[0],str[1],str[2],str[3]);
	str = hdr->fmt;
	printf("Format marker: %c%c%c%c\n", str[0],str[1],str[2],str[3]);
	printf("Format length: %d\n", hdr->subchunk1size);
	printf("Format type  : %d\n", hdr->audio_format);
	printf("Channels     : %d\n", hdr->number_of_channels);
	printf("Sample rate  : %d\n", hdr->samples_per_sec);
	printf("Bytes / sec  : %d\n", hdr->bytes_per_sec);
	printf("Bytes / Frame: %d\n", hdr->block_align);
	printf("Bits / Sample: %d\n", hdr->bits_per_sample);
	str = hdr->subchunk2id;
	printf("Data   marker: %c%c%c%c\n", str[0],str[1],str[2],str[3]);
	printf("Data length  : %d\n", hdr->subchunk2size);	
}

static int snd_pcm_init(snd_pcm_t *handle, snd_pcm_uframes_t *pFrames, unsigned int *pChannels, unsigned int *pRate)
{
	int rc;	
  	snd_pcm_hw_params_t *params;

 	// Allocate hardware parameters
  	if ((rc = snd_pcm_hw_params_malloc(&params)) < 0)
  	{
  		printf("ERROR: Cannot allocate hardware parameters. %s\n", snd_strerror(rc));
  	}

  	// Initialize parameters with default values
  	if ((rc = snd_pcm_hw_params_any(handle, params)) < 0)
  	{
  		printf("ERROR: Cannot initialize hardware parameters. %s\n", snd_strerror(rc));
  	}

  	// Setting hardware parameters
  	if ((rc = snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0)
  	{
  		printf("ERROR: Cannot set interleaved mode. %s\n", snd_strerror(rc));
  	}

  	if ((rc = snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE)) < 0)
  	{
  		printf("ERROR: Cannot set PCM format. %s\n", snd_strerror(rc));
  	}

  	if ((rc = snd_pcm_hw_params_set_period_size_near(handle, params, pFrames, 0)) < 0)
  	{
  		printf("ERROR: Cannot set period size. %s\n", snd_strerror(rc));
  	}
	
  	if ((rc = snd_pcm_hw_params_set_channels_near(handle, params, pChannels)) < 0)
  	{
  		printf("ERROR: Cannot set number of channels. %s\n", snd_strerror(rc));
  	}

 	if ((rc = snd_pcm_hw_params_set_rate_near(handle, params, pRate, 0)) < 0)
 	{
 		printf("ERROR: Cannot set plyabck rate. %s\n", snd_strerror(rc));
 	}

 	if ((rc = snd_pcm_hw_params(handle, params)) < 0)
 	{
 		printf("ERROR: Cannot set hardware parameters. %s\n", snd_strerror(rc));
 	}

 	// Get hardware parameters
 	if ((rc = snd_pcm_hw_params_get_period_size(params, pFrames, 0)) < 0)
	{
		printf("Playback ERROR: Can't get period size. %s\n", snd_strerror(rc));
	}

	if ((rc = snd_pcm_hw_params_get_channels(params, pChannels)) < 0)
	{
		printf("Playback ERROR: Can't get channel number. %s\n", snd_strerror(rc));
	}

	if ((rc = snd_pcm_hw_params_get_rate(params, pRate, 0)) < 0)
	{
		printf("ERROR: Cannot get rate. %s\n", snd_strerror(rc));
	}

	// Free paraemeters
	snd_pcm_hw_params_free(params);
	
	return 0;
}

void *sound_open(void)
{
	int rc;
 	snd_pcm_t *handle;

        // Open PCM device for playback
        if ((rc = snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0)) < 0) 
        {
         printf("ERROR: Cannot open pcm device. %s\n", snd_strerror(rc));
         return NULL;
        }

       return (void*)handle;
}

int sound_close(void *handle)
{
    if(handle)	
    snd_pcm_close((snd_pcm_t *)handle);
}

int play_wav(void *handle, unsigned char *file_buf, int length) {
    
    // Variable declaration
	int rc;
  	int buffer_size;

	int offset = 0;
	int data_len;
	
 	//snd_pcm_t *handle;
  	snd_pcm_uframes_t frames;

  	unsigned int channels;
  	unsigned int rate;
	unsigned int bytes_per_frame;

	wav_header wav_header_info;
   
	if( !handle ) return -1;

	if( !file_buf ) return -1;

	if( strncmp(file_buf,"RIFF",4) != 0 ) return -1;

	memcpy(&wav_header_info, file_buf, sizeof(wav_header));
	offset += sizeof(wav_header);
	
	//print_wav_header(&wav_header_info);

	// Assign variables that were read from the wave file
	channels = wav_header_info.number_of_channels;
	rate = wav_header_info.samples_per_sec;
	bytes_per_frame = wav_header_info.block_align;
	data_len = length - sizeof(wav_header);//wav_header_info.subchunk2size;
	
  	// Open PCM device for playback
  	//if ((rc = snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0)) < 0) 
  	//{
    	//printf("ERROR: Cannot open pcm device. %s\n", snd_strerror(rc));
  	//}
    
	frames = FRAMES;
	snd_pcm_init((snd_pcm_t *)handle, &frames, &channels, &rate);

	// Create buffer
	buffer_size = frames * bytes_per_frame * periods_per_buffer;

  	// Send info to ALSA
 	while (offset + buffer_size < data_len)
 	{
    	rc = snd_pcm_writei((snd_pcm_t *)handle, file_buf + offset, frames * periods_per_buffer);
    	if (rc == -EPIPE) 
    	{
      		snd_pcm_prepare((snd_pcm_t *)handle);
      		fprintf(stderr, "underrun occurred\n");
    	} 
    	else if (rc < 0) 
    	{
      		printf("ERROR: Cannot write to playback device. %s\n", strerror(rc));
    	}
        offset += buffer_size;
  	}

  	snd_pcm_drain((snd_pcm_t *)handle);
  	//snd_pcm_close(handle);

	return 0;
}
