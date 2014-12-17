#include "encoder.h"

#include "encoder_sndfile.h"
#include "encoder_faac.h"

supported_encoder *supported_encoders[] = {
    &sndfile_encoder,
    &faac_encoder,
    NULL
};
