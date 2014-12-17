# Copyright (c) 2009, Whispersoft s.r.l.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
# * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
# * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
# * Neither the name of Whispersoft s.r.l. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# Finds Faac library
#
#  FAAC_INCLUDE_DIR - where to find faac.h, etc.
#  FAAC_LIBRARIES   - List of libraries when using Faac.
#  FAAC_FOUND       - True if Faac found.
#

#if (FAAC_INCLUDE_DIR)
#  # Already in cache, be silent
#  set(FAAC_FIND_QUIETLY TRUE)
#endif (FAAC_INCLUDE_DIR)

find_path(FAAC_INCLUDE_DIR faac.h
  /opt/local/include
  /usr/local/include
  /usr/include
)

set(FAAC_NAMES faac)
find_library(FAAC_LIBRARY
  NAMES ${FAAC_NAMES}
  PATHS /usr/lib /usr/local/lib /opt/local/lib
)

if (FAAC_INCLUDE_DIR AND FAAC_LIBRARY)
   set(FAAC_FOUND 1)
   set(FAAC_LIBRARIES ${FAAC_LIBRARY})
else (FAAC_INCLUDE_DIR AND FAAC_LIBRARY)
   set(FAAC_FOUND FALSE)
   set(FAAC_LIBRARIES)
endif (FAAC_INCLUDE_DIR AND FAAC_LIBRARY)

if (FAAC_FOUND)
   if (NOT FAAC_FIND_QUIETLY)
      message(STATUS "Found Faac: ${FAAC_LIBRARY}")
   endif (NOT FAAC_FIND_QUIETLY)
else (FAAC_FOUND)
   if (FAAC_FIND_REQUIRED)
      message(STATUS "Looked for Faac libraries named ${FAAC_NAMES}.")
      message(STATUS "Include file detected: [${FAAC_INCLUDE_DIR}].")
      message(STATUS "Lib file detected: [${FAAC_LIBRARY}].")
      message(FATAL_ERROR "=========> Could NOT find Faac library")
   endif (FAAC_FIND_REQUIRED)
endif (FAAC_FOUND)

mark_as_advanced(
  FAAC_LIBRARY
  FAAC_INCLUDE_DIR
)

