#include <stdio.h>
#include <stdlib.h>
//#include <unistd.h>
#include <time.h>
#include <gio/gio.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <opus.h>
#include <samplerate.h>
#include "opus_header.h"
#include "circbuf.h"
#include "encoder_faac.h"

static supported_format opus_formats[] = {
    {"OPUS", "opus"},
    {NULL}
};


// GError stuff
#define GRIP_OPUSENC_ERROR opus_encoder_error_quark ()

GQuark opus_encoder_error_quark (void) {
    return g_quark_from_static_string ("grip-opusenc-error-quark");
}


// Size of data we get provided at each callback (in bytes)
#define CD_SECTOR_SIZE 2352
#define DEFAULT_FRAME_SIZE 960
#define MAX_DEFAULT_FRAME_SIZE 6*DEFAULT_FRAME_SIZE
#define MAX_PACKET_SIZE (3*1276)


enum GripOpusEncError {
//    GRIP_OPUSENC_ERROR_NOMEM,
//    GRIP_OPUSENC_ERROR_INIT,
//    GRIP_OPUSENC_ERROR_INVALIDFORMAT,
    GRIP_OPUSENC_ERROR_OUTFILE
};

enum encoding_mode {
   VBR = 1,
   CBR = 2,
   CVBR = 3     // Constrained
};

typedef struct {
    GSettings *settings;
    FILE *fout;
    guint sfreq;
    gint32 frame_size;

    ogg_stream_state   os;
    ogg_page           og;
    ogg_packet         op;
//    ogg_int64_t        last_granulepos=0;
//    ogg_int64_t        enc_granulepos=0;
//    ogg_int64_t        original_samples=0;
//    ogg_int32_t        id=-1;
//    int                last_segments=0;
//    int                eos=0;

    int skip;
	SRC_STATE *src;
	OpusEncoder *enc;
	float *src_inbuf;                             // Buffer to hold the input buffer coverted to floats to be fed to SRC
	float *src_outbuf;                          // Buffer to hold the SRC output buffer (i.e: resampled data)
	circular_buffer *enc_buffer;
	guint8 *opus_inbuf;
	guint8 *opus_outbuf;
} encoder_opus_data;


// 1 or 2
static int channels = 2;


/*Write an Ogg page to a file pointer*/
static inline int oe_write_page (ogg_page *page, FILE *fp) {
    int written;
    written = fwrite (page -> header, 1, page -> header_len, fp);
    written += fwrite (page -> body, 1, page -> body_len, fp);
    return written;
}

static gpointer encoder_opus_init (gpointer *fmt, gchar *filename, gpointer opts, GError **error) {
    g_debug ("Opus Encoder Module - Using %s", opus_get_version_string ());

    int ierror;

	encoder_opus_data *eod = g_new (encoder_opus_data, 1);

	eod -> settings = g_settings_new ("net.sukkology.software.regrip.encoders.opus");

	// Init SRC
	eod -> src = src_new (SRC_SINC_MEDIUM_QUALITY, channels, &ierror);
	if (ierror != 0) {
		src_strerror (ierror);
		g_free (eod);
		return NULL;
	}

	eod -> src_inbuf = g_new (float, CD_SECTOR_SIZE / 2);
	if (!eod -> src_inbuf) {
		eod -> src = src_delete (eod -> src);
		g_free (eod);
		return NULL;
	}

	eod -> src_outbuf = g_new (float, CD_SECTOR_SIZE / 2);
	if (!eod -> src_outbuf) {
		eod -> src = src_delete (eod -> src);
		g_free (eod);
		return NULL;
	}

	eod -> enc_buffer = cb_new (DEFAULT_FRAME_SIZE * channels * sizeof (float) * 2);
	if (!eod -> enc_buffer) {
		eod -> src = src_delete (eod -> src);
		g_free (eod);
		return NULL;
	}

	// Init Opus
	guint32 tmpfs = g_settings_get_enum (eod -> settings, "framesize") * 4.8;
	g_debug ("tmpfs = %d", tmpfs);
//	          if(strcmp(optarg,"2.5")==0)frame_size=120;
//          else if(strcmp(optarg,"5")==0)frame_size=240;
//          else if(strcmp(optarg,"10")==0)frame_size=480;
//          else if(strcmp(optarg,"20")==0)frame_size=960;
//          else if(strcmp(optarg,"40")==0)frame_size=1920;
//          else if(strcmp(optarg,"60")==0)frame_size=2880;
	eod -> sfreq = g_settings_get_enum (eod -> settings, "sampling-frequency");
	eod -> frame_size = tmpfs / (48000 / eod -> sfreq);
	int mode = eod -> frame_size < 480 / (48000 / eod -> sfreq) ? OPUS_APPLICATION_RESTRICTED_LOWDELAY : OPUS_APPLICATION_AUDIO;
	g_debug ("sfreq = %u, frame_size = %d", eod -> sfreq, eod -> frame_size);
	eod -> enc = opus_encoder_create (eod -> sfreq, channels, mode,
	                                  &ierror);
	if (ierror != OPUS_OK) {
		eod -> src = src_delete (eod -> src);
		g_free (eod);
		return NULL;
	}

	// OGG Header
    OpusHeader         header = {0};
    header.channels = channels;
//    header.channel_mapping=header.channels>8?255:chan>2;
//    header.channel_mapping = 0;
    header.input_sample_rate = 44100;
//    header.gain = 0;      /* 0 dB gain is recommended unless you know what you're doing */

    gint32 min_bytes;
    int max_frame_bytes;
    min_bytes = max_frame_bytes = (1275 * 3 + 7) * /* header.nb_streams */ 2;
    guint8 *packet = g_new (guint8, max_frame_bytes);
    if (packet == NULL){
        g_error ("Error allocating packet buffer.\n");
    }

    gint32 bitrate;
    if (g_settings_get_boolean (eod -> settings, "auto-bitrate")) {
        /*Lower default rate for sampling rates [8000-44100) by a factor of (rate+16k)/(64k)*/
        bitrate = ((64000 * /* header.nb_streams */ 2 /*+ 32000*header.nb_coupled*/) *
            (MIN (48, MAX (8, ((eod -> sfreq < 44100 ? eod -> sfreq : 48000) + 1000) / 1000)) + 16) + 32) >> 6;
    } else {
        bitrate = g_settings_get_int (eod -> settings, "bitrate") * 1000;
    }
    g_assert (bitrate <= (1024000 * channels) && bitrate >= 00);
    g_debug ("bitrate = %d", bitrate);
    ierror = opus_encoder_ctl (eod -> enc, OPUS_SET_BITRATE (bitrate));
    if (ierror != OPUS_OK) {
        g_error ("Error OPUS_SET_BITRATE returned: %s", opus_strerror (ierror));
    }

    gboolean with_hard_cbr = g_settings_get_enum (eod -> settings, "encoding-mode") == CBR;
    ierror = opus_encoder_ctl (eod -> enc, OPUS_SET_VBR (!with_hard_cbr));
    if (ierror != OPUS_OK) {
        g_error ("Error OPUS_SET_VBR returned: %s", opus_strerror (ierror));
    }

    if (!with_hard_cbr) {
        gboolean with_cvbr = g_settings_get_enum (eod -> settings, "encoding-mode") == CVBR;
        ierror = opus_encoder_ctl (eod -> enc, OPUS_SET_VBR_CONSTRAINT (with_cvbr));
        if (ierror != OPUS_OK) {
            g_error ("Error OPUS_SET_VBR_CONSTRAINT returned: %s", opus_strerror (ierror));
        }
    }

    ierror = opus_encoder_ctl (eod -> enc, OPUS_SET_COMPLEXITY (g_settings_get_int (eod -> settings, "complexity")));
    if (ierror != OPUS_OK) {
        g_error ("Error OPUS_SET_COMPLEXITY returned: %s", opus_strerror (ierror));
    }

    ierror = opus_encoder_ctl (eod -> enc, OPUS_SET_PACKET_LOSS_PERC (g_settings_get_int (eod -> settings, "expect-loss")));
    if (ierror != OPUS_OK) {
        g_error ("Error OPUS_SET_PACKET_LOSS_PERC returned: %s", opus_strerror (ierror));
    }

#ifdef OPUS_SET_LSB_DEPTH_____
  ret=opus_multistream_encoder_ctl(st, OPUS_SET_LSB_DEPTH(IMAX(8,IMIN(24,inopt.samplesize))));
  if(ret!=OPUS_OK){
    fprintf(stderr,"Warning OPUS_SET_LSB_DEPTH returned: %s\n",opus_strerror(ret));
  }
#endif


//	opus_encoder_ctl (eod -> enc, OPUS_SET_SIGNAL (OPUS_SIGNAL_VOICE));
    g_debug ("all ctls done");

    /*We do the lookahead check late so user CTLs can change it*/
    gint32 lookahead;
    ierror = opus_encoder_ctl (eod -> enc, OPUS_GET_LOOKAHEAD (&lookahead));
    if (ierror != OPUS_OK) {
        g_error ("Error OPUS_GET_LOOKAHEAD returned: %s", opus_strerror (ierror));
    }
    eod -> skip = lookahead;
    /*Regardless of the rate we're coding at the ogg timestamping/skip is
    always timed at 48000.*/
    header.preskip = eod -> skip * (48000.0 / eod -> sfreq);
    /* Extra samples that need to be read to compensate for the pre-skip */
//    inopt.extraout = (int) header.preskip * (44100.0 / 48000.0);

    // Initialize Ogg stream struct
    time_t start_time = time (NULL);
    srand (((getpid () & 65535) << 15) ^ start_time);
    int serialno = rand ();
    if (ogg_stream_init (&(eod -> os), serialno) == -1){
        g_error ("Error: stream init failed");
    }

	// Prepare output file
    gchar *extension = (gchar *) fmt;
    g_assert (extension);
	gchar *filename_ext = g_strdup_printf ("%s.%s", filename, extension);
	eod -> fout = fopen (filename_ext, "wb");
	if (!eod -> fout) {
		g_set_error (error, GRIP_OPUSENC_ERROR, GRIP_OPUSENC_ERROR_OUTFILE, _("Unable to open output file \"%s\""), filename_ext);
		g_free (filename_ext);
//		faacEncClose (eod -> codec);
		g_free (eod);
		return NULL;
	}
	g_free (filename_ext);

    /*Write header*/
    gint64 bytes_written, pages_out;
    {
        unsigned char header_data[100];
        int packet_size = opus_header_to_packet(&header, header_data, 100);
        (eod -> op).packet = header_data;
        (eod -> op).bytes = packet_size;
        (eod -> op).b_o_s = 1;
        (eod -> op).e_o_s = 0;
        (eod -> op).granulepos = 0;
        (eod -> op).packetno = 0;
        ogg_stream_packetin (&(eod -> os), &(eod -> op));

        while ((ierror = ogg_stream_flush (&(eod -> os), &(eod -> og)))) {
            if (!ierror)
                break;
            ierror = oe_write_page (&(eod -> og), eod -> fout);
            if (ierror != (eod -> og).header_len + (eod -> og).body_len) {
                g_error ("Error: failed writing header to output stream");
            }
            bytes_written += ierror;
            pages_out++;
        }

//        comment_pad(&inopt.comments, &inopt.comments_length, comment_padding);
//        op.packet=(unsigned char *)inopt.comments;
//            op.bytes = inopt.comments_length;
        (eod -> op).bytes = 0;
        (eod -> op).b_o_s = 0;
        (eod -> op).e_o_s = 0;
        (eod -> op).granulepos = 0;
        (eod -> op).packetno = 1;
        ogg_stream_packetin (&(eod -> os), &(eod -> op));
    }

    /* writing the rest of the opus header packets */
    while ((ierror = ogg_stream_flush (&(eod -> os), &(eod -> og)))) {
        if (!ierror)
            break;
        ierror = oe_write_page (&(eod -> og), eod -> fout);
        if (ierror != (eod -> og).header_len + (eod -> og).body_len) {
            g_error ("Error: failed writing header to output stream");
        }
        bytes_written += ierror;
        pages_out++;
    }

    g_debug ("encoder_opus_init() returning");

    return eod;
}

static gboolean encoder_opus_callback (gint16 *buffer, gsize bufsize, gpointer user_data) {
	encoder_opus_data *eod = (encoder_opus_data *) user_data;
	g_assert (eod);

	g_debug ("encoder_opus_callback()");

	// Convert input buffer to float, as SRC only accepts it this way
	g_assert (bufsize / 2 <= CD_SECTOR_SIZE / 2);
	src_short_to_float_array (buffer, eod -> src_inbuf, bufsize / 2);

	SRC_DATA data;
	data.data_in = eod -> src_inbuf;
	data.data_out = eod -> src_outbuf;
	data.input_frames = CD_SECTOR_SIZE / 2 / channels;                           //  the length of the array divided by the number of channels
	data.output_frames = CD_SECTOR_SIZE  / 2 / channels;                         //  the length of the array divided by the number of channels
	//long   input_frames_used, output_frames_gen ;
	data.end_of_input = 0;
	data.src_ratio  = (float) eod -> sfreq / 44100;

	int ret;

	if ((ret = src_process (eod -> src, &data)) < 0) {
		g_error ("OPUS: %s", src_strerror (ret));
	} else {
	    g_debug ("SRC consumed %ld frames and generated %ld frames", data.input_frames_used, data.output_frames_gen);
	    int x = cb_write_block (eod -> enc_buffer, (guint8 *) eod -> src_outbuf, data.output_frames_gen * sizeof (float));
	    g_assert (x);
	}

	// To encode a frame, opus_encode() or opus_encode_float() must be called with exactly one frame (2.5, 5, 10, 20, 40 or 60 ms) of audio data:
	guint frame_size = DEFAULT_FRAME_SIZE * channels * sizeof (float);
	if (cb_used (eod -> enc_buffer) >= frame_size) {
        float in[DEFAULT_FRAME_SIZE * channels];
        guint8 out[MAX_PACKET_SIZE];
        cb_read_block (eod -> enc_buffer, (guint8 *) in, frame_size);
        int len = opus_encode_float (eod -> enc, in, DEFAULT_FRAME_SIZE, out, MAX_PACKET_SIZE);
        g_debug ("len = %d", len);
        /*where

        audio_frame is the audio data in opus_int16 (or float for opus_encode_float())
        frame_size is the duration of the frame in samples (per channel)
        packet is the byte array to which the compressed data is written
        max_packet is the maximum number of bytes that can be written in the packet (4000 bytes is recommended). Do not use max_packet to control VBR target bitrate, instead use the OPUS_SET_BITRATE CTL.
        opus_encode() and opus_encode_float() return the number of bytes actually written to the packet. The return value can be negative, which indicates that an error has occurred. If the return value is 1 byte, then the packet does not need to be transmitted (DTX). */
        if (len < 0) {
            g_warning ("opus_encode_float() failed: %s", opus_strerror (len));
            return FALSE;
        } else {
            size_t n = fwrite (out, sizeof (guint8), len, eod -> fout);
            if (n != len) {
                g_warning (_("Unable to write to output file"));
                return FALSE;
            } else {
                g_debug ("Wrote %d bytes", len);
            }
        }
    }

    return TRUE;
}

static gboolean encoder_opus_close (gpointer user_data, GError **error) {
	encoder_opus_data *eod = (encoder_opus_data *) user_data;
	g_assert (eod);

	// Close output file
	fclose (eod -> fout);

	// Close SRC
	g_free (eod -> src_inbuf);
	eod -> src = src_delete (eod -> src);

	// Close Opus
	opus_encoder_destroy (eod -> enc);

	g_free (eod);

	return TRUE;
}

// Encoder registration
supported_encoder opus_encoder = {
    "Opus Encoder",
    opus_formats,
    encoder_opus_init,
    encoder_opus_close,
    encoder_opus_callback
};
