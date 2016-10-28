# Configures the compiler for building the host given the target environment etc.

IF ( MSVC )

    INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_SOURCE_DIR}/../proj/win )

    ADD_DEFINITIONS(
        -D_CRT_SECURE_NO_WARNINGS=1
        -D_CRT_NONSTDC_NO_DEPRECATE=1
        -DWIN32
        -DUNICODE
        -D_UNICODE
        -D_HAS_ITERATOR_DEBUGGING=0
        -D_SECURE_SCL=0
    )
    ADD_COMPILE_OPTIONS(
        "/Zi" "/Zc:wchar_t" "/Zc:forScope" "/EHsc" "/fp:precise"
        "/WX-" "/FR" "/wd4480" "/wd4800"
        "/nologo" "/Gm"
    )
	IF ( CMAKE_BUILD_TYPE STREQUAL Debug )
		ADD_COMPILE_OPTIONS( "/MTd" )
		ADD_DEFINITIONS( "-DDEBUG=1" )
	ELSE()
		ADD_COMPILE_OPTIONS( "/MT" )
		ADD_DEFINITIONS( "-DNDEBUG=1" )
	ENDIF()

    IF ( BUILD_BITNESS EQUAL 64 )
        ADD_COMPILE_OPTIONS( "/arch:AVX" )
    ELSE()
        ADD_COMPILE_OPTIONS( "/arch:SSE2" )
    ENDIF()

    IF ( CMAKE_BUILD_TYPE MATCHES Debug )
        ADD_COMPILE_OPTIONS( "/Oy-" "/GF-" "/GS" "/Gy" "/Od" )
    ELSE()
        ADD_COMPILE_OPTIONS( "/Ob2" "/Oi" "/Ot" "/Oy" "/GT" "/GF" "/GS-" "/Gy-" "/Ox" "/GL" "/MP" )
        LINK_LIBRARIES( "/OPT:REF" "/OPT:ICF" "/LTCG" )
    ENDIF()

ELSE()

    ADD_COMPILE_OPTIONS( "-Winvalid-pch" "-Wno-array-bounds" )
    ADD_COMPILE_OPTIONS( "-Wno-strict-aliasing" "-Wno-unused-result" "-Wno-unknown-pragmas" )
    ADD_COMPILE_OPTIONS( "-Wall" "-Wshadow" "-Wno-deprecated" )

    IF ( NOT CMAKE_CXX_COMPILER_ID MATCHES Clang )
        # Use cheap list of stupid options for GCC
    ELSE()
        # Every warning ever, then shut off the ones we don't want
        ADD_COMPILE_OPTIONS( "-Weverything" )
        ADD_COMPILE_OPTIONS( "-Wno-double-promotion" "-Wno-cstring-format-directive" )
        ADD_COMPILE_OPTIONS( "-Wno-error=c++11-narrowing" "-Wno-packed" "-Wno-pedantic" )
        ADD_COMPILE_OPTIONS( "-Wno-gnu-anonymous-struct" "-Wno-cast-align" )
        ADD_COMPILE_OPTIONS( "-Wno-covered-switch-default" "-Wno-exit-time-destructors" )
        ADD_COMPILE_OPTIONS( "-Wno-documentation" "-Wno-unused-parameter" "-Wno-c++98-compat" )
        ADD_COMPILE_OPTIONS( "-Wno-padded" "-Wno-undef" "-Wno-c++98-compat-pedantic" )
        ADD_COMPILE_OPTIONS( "-Wno-used-but-marked-unused" "-Wno-c99-extensions" )
        ADD_COMPILE_OPTIONS( "-Wno-disabled-macro-expansion" "-Wno-unused-macros" )
        ADD_COMPILE_OPTIONS( "-Wno-unused-exception-parameter" "-Wno-global-constructors" )
        ADD_COMPILE_OPTIONS( "-Wno-switch-enum" "-Wno-gnu-zero-variadic-macro-arguments" )
        ADD_COMPILE_OPTIONS( "-Wno-variadic-macros" "-Wno-missing-noreturn" )
        ADD_COMPILE_OPTIONS( "-Wno-unknown-warning-option" "-Wno-auto-import" )
        ADD_COMPILE_OPTIONS( "-Wno-overlength-strings" "-Wno-assign-enum" )
        ADD_COMPILE_OPTIONS( "-Wno-objc-missing-property-synthesis" )
        ADD_COMPILE_OPTIONS( "-Wno-gnu-conditional-omitted-operand" "-Wno-old-style-cast" )
        ADD_COMPILE_OPTIONS( "-Wno-documentation-unknown-command" "-Wno-reserved-id-macro" )
        ADD_COMPILE_OPTIONS( "-Wno-conversion" "-Wno-weak-vtables" "-Wno-format-nonliteral" )
        ADD_COMPILE_OPTIONS( "-Wno-missing-prototypes" "-Wno-missing-variable-declarations" )
        ADD_COMPILE_OPTIONS( "-Wno-implicit-fallthrough" )
        ADD_COMPILE_OPTIONS( "-Wno-date-time" )
        ADD_COMPILE_OPTIONS( "-stdlib=libc++" )
        LINK_LIBRARIES( "-stdlib=libc++" )
    ENDIF()

    ADD_COMPILE_OPTIONS( "-fstack-protector-all" )

    if ( CMAKE_BUILD_TYPE MATCHES Debug )
        ADD_COMPILE_OPTIONS( "-O0" )
        ADD_DEFINITIONS( "-DDEBUG=1" )
		# ADD_COMPILE_OPTIONS( "-fsanitize=address" )
        # LINK_LIBRARIES( "-fsanitize=address" )
    ELSE()
        ADD_COMPILE_OPTIONS( "-Os" )
        ADD_DEFINITIONS( "-DNDEBUG=1" )
    ENDIF()
    LINK_LIBRARIES( "-lpthread" )

    IF ( BUILD_BITNESS EQUAL 64 )
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m64" )
		SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m64" )
	ELSE()
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32" )
		SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32" )
    ENDIF()

ENDIF()

# Let the user specify overriding flags
SET( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${PN_CXX_FLAGS}" )
