#include "encoder.h"
#include "config.h"

#include "encoder_sndfile.h"
#include "encoder_faac.h"

supported_encoder *supported_encoders[] = {
#ifdef SNDFILE_FOUND
    &sndfile_encoder,
#endif // SNDFILE_FOUND

#ifdef FAAC_FOUND
    &faac_encoder,
#endif // FAAC_FOUND

    NULL
};
