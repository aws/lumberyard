CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

PROJECT (HLSLCrossCompilerProj C CXX)

IF(NOT CMAKE_BUILD_TYPE)
	SET(CMAKE_BUILD_TYPE "Release")
ENDIF(NOT CMAKE_BUILD_TYPE)

IF( ${CMAKE_BUILD_TYPE} STREQUAL Debug )
    # CMAKE_C_FLAGS_DEBUG will also be added to CMAKE_C_FLAGS
    MESSAGE( "Debug build" )
    ADD_DEFINITIONS(-DDEBUG -D_DEBUG -D__DEBUG__)
    SET(CMAKE_DEBUG_POSTFIX "_d")
ENDIF()

IF( BUILD_SHARED_LIBS)
   MESSAGE("Dynamic libs")
   ADD_DEFINITIONS(-DHLSLCC_DYNLIB)
ENDIF( BUILD_SHARED_LIBS )

IF(UNIX AND NOT APPLE)
    IF(STEAMOS)
    	SET( CMAKE_ARCHIVE_OUTPUT_DIRECTORY "../lib/steamos" )
    	SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY "../bin/steamos" )
		ELSE()
    	SET( CMAKE_ARCHIVE_OUTPUT_DIRECTORY "../lib/linux" )
    	SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY "../bin/linux" )
		ENDIF()
ELSEIF(APPLE AND IOS)
    SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY "../bin/ios" )
    SET( CMAKE_ARCHIVE_OUTPUT_DIRECTORY "../lib/ios" )
ELSEIF(APPLE)
    SET( CMAKE_ARCHIVE_OUTPUT_DIRECTORY "../lib/mac" )
    SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY "../bin/mac" )
ELSEIF(WIN32)
    IF (CMAKE_CL_64 OR CMAKE_GENERATOR MATCHES "Win64")
        SET( CMAKE_ARCHIVE_OUTPUT_DIRECTORY "../lib/win64" )
        SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY "../bin/win64" )
    ELSE()
        SET( CMAKE_ARCHIVE_OUTPUT_DIRECTORY "../lib/win32" )
        SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY "../bin/win32" )
    ENDIF()
ELSE()
    MESSAGE(FATAL_ERROR "Unknown platform detected")
ENDIF()


SET( CMAKE_CFG_INTDIR "." )


#INCLUDE(TestBigEndian)

#TEST_BIG_ENDIAN(BIG_ENDIAN_SYSTEM)

IF(BIG_ENDIAN_SYSTEM)
    MESSAGE("SETTING Big endian")
    ADD_DEFINITIONS(-D__BIG_ENDIAN__)
ELSE()
    MESSAGE("SETTING Little endian")
    ADD_DEFINITIONS(-D__LITTLE_ENDIAN__)
ENDIF()

IF(MSVC)
	OPTION(USE_MSVC_FAST_FLOATINGPOINT "Use MSVC /fp:fast option" ON)
	IF(USE_MSVC_FAST_FLOATINGPOINT)
		ADD_DEFINITIONS(/fp:fast)
	ENDIF(USE_MSVC_FAST_FLOATINGPOINT)
	OPTION(PORTABLE "Remove dependencies from OpenGL and C runtime" OFF)
	IF(PORTABLE)
		SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}_portable")
		SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}_portable")
		ADD_DEFINITIONS(-DPORTABLE)
		foreach(flag_var
			CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
			CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO
			CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
			CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO)
			if(${flag_var} MATCHES "/MD")
				string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
			endif(${flag_var} MATCHES "/MD")
		endforeach(flag_var)
	ENDIF(PORTABLE)
ENDIF(MSVC)

IF(WIN32)
    ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS /WX /W3 /arch:SSE2)
ENDIF()

INCLUDE_DIRECTORIES( ../include ../src/cbstring/ ../src/ )

file(GLOB CBSTRING_CFILES ../src/cbstring/*.c)
file(GLOB CBSTRING_HFILES ../src/cbstring/*.h)
source_group("CBString Files" FILES ${CBSTRING_CFILES} ${CBSTRING_HFILES})

if(UNIX)
    add_definitions("-fPIC")
endif()

SET(libHLSLcc_SOURCES  
../src/internal_includes/debug.h
../src/internal_includes/decode.h
../src/internal_includes/languages.h
../include/pstdint.h
../src/internal_includes/reflect.h
../src/internal_includes/structs.h
../include/hlslcc.h
../include/hlslcc_bin.hpp
../src/internal_includes/toGLSLDeclaration.h
../src/internal_includes/toGLSLInstruction.h
../src/internal_includes/toGLSLOperand.h
../src/internal_includes/tokens.h
../src/internal_includes/tokensDX9.h
../src/internal_includes/shaderLimits.h
../src/internal_includes/hlslcc_malloc.h
../src/internal_includes/hlslcc_malloc.c
${CBSTRING_HFILES}
../src/decode.c
../src/decodeDX9.c
../src/reflect.c
../src/toGLSL.c
../src/toGLSLInstruction.c
../src/toGLSLOperand.c
../src/toGLSLDeclaration.c
${CBSTRING_CFILES}
)
ADD_LIBRARY( libHLSLcc ${libHLSLcc_SOURCES} )
	

SET_TARGET_PROPERTIES (libHLSLcc PROPERTIES PREFIX "")

INCLUDE_DIRECTORIES( ../offline/cjson/ )

file(GLOB CJSON_CFILES ../offline/cjson/*.c)
file(GLOB CJSON_HFILES ../offline/cjson/*.h)
source_group("cJSON Files" FILES ${CJSON_CFILES} ${CJSON_HFILES})

SET(HLSLcc_SOURCES  
../offline/compilerStandalone.cpp
../offline/timer.h
../offline/timer.cpp
../offline/hash.h
../offline/serializeReflection.cpp
../offline/serializeReflection.h
${CJSON_CFILES}
${CJSON_HFILES}
)

ADD_EXECUTABLE( HLSLcc ${HLSLcc_SOURCES} )

# Compile 32-bit binaries for linux
IF(CMAKE_HOST_UNIX)

ADD_LIBRARY( libHLSLcc-i386 ${libHLSLcc_SOURCES} )
ADD_EXECUTABLE( HLSLcc-i386 ${HLSLcc_SOURCES} )
TARGET_LINK_LIBRARIES( HLSLcc-i386 libHLSLcc-i386)
SET_TARGET_PROPERTIES(libHLSLcc-i386 PROPERTIES COMPILE_FLAGS -m32 LINK_FLAGS -m32 )
SET_TARGET_PROPERTIES(HLSLcc-i386 PROPERTIES COMPILE_FLAGS -m32 LINK_FLAGS -m32 )
ENDIF()

TARGET_LINK_LIBRARIES( HLSLcc libHLSLcc)

# force variables that could be defined on the cmdline
# to be written to the cach

SET( CMAKE_BUILD_TYPE "${CMAKE_BUILD_TYPE}" CACHE STRING
"One of None Debug Release RelWithDebInfo MinSizeRel" FORCE )

