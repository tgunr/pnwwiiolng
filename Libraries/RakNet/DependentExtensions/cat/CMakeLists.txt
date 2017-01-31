cmake_minimum_required(VERSION 2.6) # or other version

project(LIBCAT)

# Define some shortcuts
get_filename_component(PARENT_DIR ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)

IF(WIN32 AND NOT UNIX)
	SET( CMAKE_CXX_FLAGS "/DWIN32 /D_WINDOWS /W0 /D_CRT_NONSTDC_NO_DEPRECATE /D_CRT_SECURE_NO_DEPRECATE /GS- /GR- -w")
	IF( ${MSVC_VERSION} STRGREATER "1500" )
		SET( CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} " /EHsc " )
	ENDIF()
ENDIF(WIN32 AND NOT UNIX)

SET(SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/)
SET(LIB ${CMAKE_CURRENT_SOURCE_DIR}/lib/)
SET(INC ${PARENT_DIR})
SET(TESTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/)

MESSAGE ( STATUS "Include path: " ${INC} )

include_directories(${INC})
link_directories(${LIB}/cat)
#set(LIBRARY_OUTPUT_PATH ${LIB}/cat)

# Common
add_library(libcatcommon STATIC
${SRC}/port/EndianNeutral.cpp
${SRC}/port/AlignedAlloc.cpp
${SRC}/threads/Mutex.cpp
${SRC}/threads/RegionAllocator.cpp
${SRC}/threads/RWLock.cpp
${SRC}/threads/Thread.cpp
${SRC}/threads/ThreadPool.cpp
${SRC}/threads/WaitableFlag.cpp
${SRC}/time/Clock.cpp
${SRC}/lang/Strings.cpp
${SRC}/io/Base64.cpp
${SRC}/io/Logging.cpp
${SRC}/io/MMapFile.cpp
${SRC}/io/Settings.cpp
${SRC}/io/ThreadPoolFiles.cpp
${SRC}/rand/MersenneTwister.cpp
${SRC}/rand/StdRand.cpp
${SRC}/parse/BitStream.cpp
${SRC}/parse/BufferTok.cpp
${SRC}/parse/MessageRouter.cpp
${SRC}/hash/MurmurHash2.cpp)
if (WIN32)
    target_link_libraries(libcatcommon winmm.lib)
else (WIN32)
    target_link_libraries(libcatcommon pthread)
endif (WIN32)

# Codec
add_library(libcatcodec STATIC
${SRC}/codec/RangeCoder.cpp)
target_link_libraries(libcatcodec libcatcommon)

# Crypt
add_library(libcatcrypt STATIC
${SRC}/crypt/privatekey/ChaCha.cpp
${SRC}/crypt/cookie/CookieJar.cpp
${SRC}/crypt/rand/EntropyLinux.cpp
${SRC}/crypt/rand/EntropyWindows.cpp
${SRC}/crypt/rand/EntropyWindowsCE.cpp
${SRC}/crypt/rand/EntropyGeneric.cpp
${SRC}/crypt/rand/Fortuna.cpp
${SRC}/crypt/hash/HMAC_MD5.cpp
${SRC}/crypt/hash/Skein.cpp
${SRC}/crypt/hash/Skein256.cpp
${SRC}/crypt/hash/Skein512.cpp
${SRC}/crypt/SecureCompare.cpp)
target_link_libraries(libcatcrypt libcatcommon)
if (WIN32)
    target_link_libraries(libcatcrypt rpcrt4.lib iphlpapi.lib psapi.lib advapi32.lib)
endif (WIN32)

# Math
add_library(libcatmath STATIC
${SRC}/math/BigRTL.cpp
${SRC}/math/BigPseudoMersenne.cpp
${SRC}/math/BigTwistedEdwards.cpp
${SRC}/math/BigMontgomery.cpp)
target_link_libraries(libcatmath libcatcommon)

if (NOT CMAKE_SIZEOF_VOID_P EQUAL 4)

    # x64-bit assembly code
    if (WIN32)
        set(big_x64_files ${LIB}/cat/big_x64.obj)
    else (WIN32)
        set(big_x64_files ${LIB}/cat/big_x64.o)
    endif (WIN32)

    # wrap the pre-built code in a static link library
    add_library(big_x64_obj STATIC EXCLUDE_FROM_ALL ${big_x64_files})
    set_source_files_properties(${big_x64_files} PROPERTIES EXTERNAL_OBJECT true GENERATED true)
    set_target_properties(big_x64_obj PROPERTIES LINKER_LANGUAGE C ARCHIVE_OUTPUT_DIRECTORY ${LIB}/cat)

    target_link_libraries(libcatmath big_x64_obj)

endif (NOT CMAKE_SIZEOF_VOID_P EQUAL 4)

# Tunnel
add_library(libcattunnel STATIC
${SRC}/crypt/tunnel/KeyMaker.cpp
${SRC}/crypt/tunnel/KeyAgreement.cpp
${SRC}/crypt/tunnel/KeyAgreementInitiator.cpp
${SRC}/crypt/tunnel/KeyAgreementResponder.cpp
${SRC}/crypt/tunnel/EasyHandshake.cpp
${SRC}/crypt/tunnel/AuthenticatedEncryption.cpp)
target_link_libraries(libcattunnel libcatcrypt libcatmath)

