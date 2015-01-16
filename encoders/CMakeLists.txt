option (ENABLE_FAAC "Enable FAAC AAC encoder" ON)
if (ENABLE_FAAC)
    add_subdirectory (faac)
endif (ENABLE_FAAC)

option (ENABLE_LAME "Enable LAME MP3 encoder" ON)
if (ENABLE_LAME)
    add_subdirectory (lame)
endif (ENABLE_LAME)

option (ENABLE_SNDFILE "Enable sndfile WAV/OGG/FLAC encoder" ON)
if (ENABLE_SNDFILE)
    add_subdirectory (sndfile)
endif (ENABLE_SNDFILE)

# Not working correctly
option (ENABLE_OPUS "Enable OPUS encoder" OFF)
if (ENABLE_OPUS)
    add_subdirectory (opus)
endif (ENABLE_OPUS)
