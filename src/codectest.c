#include <stdio.h>
#include <string.h>
#include <faac.h>
#include <stdlib.h>
#include <sndfile.h>
#include "circbuf.h"

#ifndef TESTENC
    #error "TESTENC not defined"
#endif // TESTENC

// This includes the relevant encoder_XXX.h file
// See https://git.kernel.org/cgit/linux/kernel/git/torvalds/linux.git/tree/include/linux/compiler-gcc.h#n103
#define __GET_HEADER(x) #x
#define _GET_HEADER(x) __GET_HEADER(encoder_##x.h)
#define GET_HEADER(x) _GET_HEADER(x)
#include GET_HEADER(TESTENC)

#define _ENCODER_STRUCT(x) (x ## _encoder)
#define ENCODER_STRUCT(x) _ENCODER_STRUCT(x)

// Bytes
#define CD_SECTOR_SIZE 2352

#define INPUT_FILE "file.wav"


int main () {
	SNDFILE *sf;
	SF_INFO info;
	int num;

	/* Open the WAV file. */
	info.format = 0;
	sf = sf_open (INPUT_FILE, SFM_READ, &info);
	if (sf == NULL) {
		g_error ("Failed to open input file '%s'", INPUT_FILE);
	}

	/* Print some of the info, and figure out how much data to read. */
	g_message ("Input file info:");
	g_message ("frames = %d", (int) info.frames);
	g_message ("samplerate = %d", info.samplerate);
	g_message ("channels = %d", info.channels);
//	g_message ("num_items=%d", num_items);

    supported_encoder *encoder = &ENCODER_STRUCT(TESTENC);
//    supported_encoder *encoder = &faac_encoder;
    supported_format *fmt = &(encoder -> supported_formats[0]);

    g_message ("Testing '%s' with format '%s'", encoder -> name, fmt -> name);

    GError *error = NULL;
    gpointer encdata;
	if (!(encdata = encoder -> init (fmt -> data, "testout", NULL, &error))) {
        g_assert (error != NULL);
        g_error ("Cannot init encoder: %s", error -> message);
	}

	do {
		gint16 buffer[CD_SECTOR_SIZE / 2];
		num = sf_read_short (sf, buffer, CD_SECTOR_SIZE / 2);
//		printf ("Read %d 16-bit items\n", num);

        if (!encoder -> callback (buffer, num * 2, encdata)) {
            g_assert (error != NULL);
            g_error ("Encoder callback failed: %s" , error -> message);
        }
	} while (num == CD_SECTOR_SIZE / 2);

	error = NULL;
	if (!encoder -> close (encdata, &error)) {
        g_assert (error != NULL);
        g_error ("Encoder close failed: %s" , error -> message);
	}

	sf_close (sf);

	return 0;
}

