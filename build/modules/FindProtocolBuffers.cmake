#Sirikata
#FindProtocolBuffers.cmake
#
#Copyright (c) 2009, Ewen Cheslack-Postava
#All rights reserved.
#
#Redistribution and use in source and binary forms, with or without
#modification, are permitted provided that the following conditions are met:
#
#    * Redistributions of source code must retain the above copyright notice,
#      this list of conditions and the following disclaimer.
#    * Redistributions in binary form must reproduce the above copyright notice,
#      this list of conditions and the following disclaimer in the documentation
#      and/or other materials provided with the distribution.
#    * Neither the name of the Sirikata nor the names of its contributors
#      may be used to endorse or promote products derived from this software
#      without specific prior written permission.
#
#THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
#ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
#WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
#DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
#ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
#(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
#LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
#ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
#SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# Try to find PROTOCOLBUFFERS include dirs and libraries
# Usage of this module is as follows:
# SET(PROTOCOLBUFFERS_ROOT /path/to/root)
# FIND_PACKAGE(ProtocolBuffers)
#
# Variables defined by this module:
#
#   PROTOCOLBUFFERS_FOUND              Set to true to indicate the library was found
#   PROTOCOLBUFFERS_INCLUDE_DIRS       The directory containing the protocol buffers header files
#   PROTOCOLBUFFERS_LIBRARIES          The libraries needed to use protocol buffers (without the full path)
#   PROTOCOLBUFFERS_COMPILER           The protocol buffers compiler
#   PROTOCOLBUFFERS_MONO_COMPILER      The protocol buffers mono compiler
#
#   PROTOCOLBUFFERS_SUPPORTS_CSHARP    True if this compiler supports C#
#
# Copyright (C) Ewen Cheslack-Postava, 2009
#

# Look for the protocol buffers headers, first in the additional location and then in default system locations
FIND_PATH(PROTOCOLBUFFERS_INCLUDE_DIRS NAMES google/protobuf/message.h PATHS ${PROTOCOLBUFFERS_ROOT} ${PROTOCOLBUFFERS_ROOT}/include/ DOC "Location of protocol buffers header files" NO_DEFAULT_PATH)
IF(NOT PROTOCOLBUFFERS_INCLUDE_DIRS)
    FIND_PATH(PROTOCOLBUFFERS_INCLUDE_DIRS NAMES google/protobuf/message.h DOC "Location of protocol buffers header files")
ENDIF()

SET(PROTOCOLBUFFERS_FOUND FALSE)

IF(PROTOCOLBUFFERS_INCLUDE_DIRS)
    # toplevel directory
    SET(PROTOCOLBUFFERS_ROOT_DIRS ${PROTOCOLBUFFERS_INCLUDE_DIRS})
    IF("${PROTOCOLBUFFERS_ROOT_DIRS}" MATCHES "/include$")
        # Destroy trailing "/include" in the path.
        GET_FILENAME_COMPONENT(PROTOCOLBUFFERS_ROOT_DIRS ${PROTOCOLBUFFERS_ROOT_DIRS} PATH)
    ENDIF()

    # compiler path
    SET(PROTOCOLBUFFERS_BIN_DIRS ${PROTOCOLBUFFERS_ROOT_DIRS})
    IF(EXISTS "${PROTOCOLBUFFERS_BIN_DIRS}/bin")
        SET(PROTOCOLBUFFERS_BIN_DIRS ${PROTOCOLBUFFERS_BIN_DIRS}/bin)
    ENDIF()
    
    # compiler inside binary directory
    FIND_FILE(PROTOCOLBUFFERS_COMPILER NAMES protoc protoc.bin protoc.exe PATHS ${PROTOCOLBUFFERS_BIN_DIRS} NO_DEFAULT_PATH)
    IF(NOT PROTOCOLBUFFERS_COMPILER)
        FIND_FILE(PROTOCOLBUFFERS_COMPILER NAMES protoc protoc.bin protoc.exe PATHS ${PROTOCOLBUFFERS_BIN_DIRS})
    ENDIF()
    FIND_FILE(PROTOCOLBUFFERS_MONO_COMPILER NAMES ProtoGen.exe PATHS ${PROTOCOLBUFFERS_BIN_DIRS} NO_DEFAULT_PATH)
    IF(NOT PROTOCOLBUFFERS_MONO_COMPILER)
        FIND_FILE(PROTOCOLBUFFERS_MONO_COMPILER NAMES ProtoGen.exe PATHS ${PROTOCOLBUFFERS_BIN_DIRS})
    ENDIF()
    
    # check if compiler supports csharp
    IF(PROTOCOLBUFFERS_COMPILER)
      # FIXME we should have a better way to determine this, e.g. by running the compiler
      FIND_FILE(PROTOCOLBUFFERS_CSHARP_LIB NAMES Google.ProtocolBuffers.dll PATHS ${PROTOCOLBUFFERS_BIN_DIRS} NO_DEFAULT_PATH)
      IF(PROTOCOLBUFFERS_CSHARP_LIB AND PROTOCOLBUFFERS_MONO_COMPILER)
        SET(PROTOCOLBUFFERS_SUPPORTS_CSHARP TRUE)
      ENDIF()
    ENDIF()

    # library path
    SET(PROTOCOLBUFFERS_LIBRARY_DIRS ${PROTOCOLBUFFERS_ROOT_DIRS})
    IF(EXISTS "${PROTOCOLBUFFERS_LIBRARY_DIRS}/lib")
        SET(PROTOCOLBUFFERS_LIBRARY_DIRS ${PROTOCOLBUFFERS_LIBRARY_DIRS}/lib)
    ENDIF(EXISTS "${PROTOCOLBUFFERS_LIBRARY_DIRS}/lib")

    IF(WIN32) # must distinguish debug and release builds
        FIND_LIBRARY(PROTOCOLBUFFERS_DEBUG_LIBRARY NAMES protobufd protobuf_d libprotobufd libprotobuf_d
                                 PATH_SUFFIXES "" Debug PATHS ${PROTOCOLBUFFERS_LIBRARY_DIRS} NO_DEFAULT_PATH)
        FIND_LIBRARY(PROTOCOLBUFFERS_RELEASE_LIBRARY NAMES protobuf libprotobuf
                                 PATH_SUFFIXES "" Release PATHS ${PROTOCOLBUFFERS_LIBRARY_DIRS} NO_DEFAULT_PATH)

        SET(PROTOCOLBUFFERS_LIBRARIES)
        IF(PROTOCOLBUFFERS_DEBUG_LIBRARY AND PROTOCOLBUFFERS_RELEASE_LIBRARY)
            SET(PROTOCOLBUFFERS_LIBRARIES debug ${PROTOCOLBUFFERS_DEBUG_LIBRARY} optimized ${PROTOCOLBUFFERS_RELEASE_LIBRARY})
        ELSEIF(PROTOCOLBUFFERS_DEBUG_LIBRARY)
            SET(PROTOCOLBUFFERS_LIBRARIES ${PROTOCOLBUFFERS_DEBUG_LIBRARY})
        ELSEIF(PROTOCOLBUFFERS_RELEASE_LIBRARY)
            SET(PROTOCOLBUFFERS_LIBRARIES ${PROTOCOLBUFFERS_RELEASE_LIBRARY})
        ENDIF()
    ELSE()
        FIND_LIBRARY(PROTOCOLBUFFERS_LIBRARIES NAMES protobuf PATHS ${PROTOCOLBUFFERS_LIBRARY_DIRS} NO_DEFAULT_PATH)
        IF(NOT PROTOCOLBUFFERS_LIBRARIES)
            FIND_LIBRARY(PROTOCOLBUFFERS_LIBRARIES NAMES protobuf PATHS ${PROTOCOLBUFFERS_LIBRARY_DIRS})
        ENDIF()
    ENDIF()

    IF(PROTOCOLBUFFERS_LIBRARIES AND PROTOCOLBUFFERS_COMPILER)
        SET(PROTOCOLBUFFERS_FOUND TRUE)
    ENDIF()
ENDIF()

IF(PROTOCOLBUFFERS_FOUND)
    IF(NOT PROTOCOLBUFFERS_FIND_QUIETLY)
        MESSAGE(STATUS "Protocol Buffers: includes at ${PROTOCOLBUFFERS_INCLUDE_DIRS}, libraries at ${PROTOCOLBUFFERS_LIBRARIES}, binary at ${PROTOCOLBUFFERS_COMPILER}")
    ENDIF()
ELSE()
    IF(PROTOCOLBUFFERS_FIND_REQUIRED)
        MESSAGE(FATAL_ERROR "ProtocolBuffers not found")
    ENDIF()
ENDIF()
