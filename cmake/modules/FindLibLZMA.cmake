# - Find LibLZMA
# Find LibLZMA headers and library
#
#  LIBLZMA_FOUND             - True if liblzma is found.
#  LIBLZMA_INCLUDE_DIR       - Directory where liblzma headers are located.
#  LIBLZMA_LIBRARIES         - Lzma libraries to link against.
#  LIBLZMA_HAS_AUTO_DECODER  - True if lzma_auto_decoder() is found (required).
#  LIBLZMA_HAS_EASY_ENCODER  - True if lzma_easy_encoder() is found (required).
#  LIBLZMA_HAS_LZMA_PRESET   - True if lzma_lzma_preset() is found (required).


# Copyright (c) 2008, Per Øyvind Karlsen, <peroyvind@mandriva.org>


IF (LIBLZMA_INCLUDE_DIRS AND LIBLZMA_LIBRARIES)
    SET(LIBLZMA_FIND_QUIETLY TRUE)
ENDIF (LIBLZMA_INCLUDE_DIRS AND LIBLZMA_LIBRARIES)

IF (NOT WIN32)
   INCLUDE(FindPkgConfig)
   PKG_SEARCH_MODULE(LIBLZMA liblzma)
   ELSE (NOT WIN32)
       FIND_PATH(LIBLZMA_INCLUDE_DIRS lzma.h )
       FIND_LIBRARY(LIBLZMA_LIBRARIES NAMES lzma )
       INCLUDE(FindPackageHandleStandardArgs)
       FIND_PACKAGE_HANDLE_STANDARD_ARGS(LIBLZMA DEFAULT_MSG LIBLZMA_INCLUDE_DIRS LIBLZMA_LIBRARIES)
ENDIF (NOT WIN32)

IF (LIBLZMA_FOUND)
   INCLUDE(CheckLibraryExists)
   CHECK_LIBRARY_EXISTS(${LIBLZMA_LIBRARIES} lzma_auto_decoder "" LIBLZMA_HAS_AUTO_DECODER)
   CHECK_LIBRARY_EXISTS(${LIBLZMA_LIBRARIES} lzma_easy_encoder "" LIBLZMA_HAS_EASY_ENCODER)
   CHECK_LIBRARY_EXISTS(${LIBLZMA_LIBRARIES} lzma_lzma_preset "" LIBLZMA_HAS_LZMA_PRESET)
   IF (NOT LIBLZMA_HAS_AUTO_DECODER AND LIBLZMA_HAS_EASY_ENCODER AND LIBLZMA_HAS_LZMA_PRESET)
      SET(LIBLZMA_FOUND FALSE)
   ENDIF(NOT LIBLZMA_HAS_AUTO_DECODER AND LIBLZMA_HAS_EASY_ENCODER AND LIBLZMA_HAS_LZMA_PRESET)
ENDIF (LIBLZMA_FOUND)

MARK_AS_ADVANCED( LIBLZMA_INCLUDE_DIRS LIBLZMA_LIBRARIES )