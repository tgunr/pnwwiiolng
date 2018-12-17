# Configures the compiler for building the host given the target environment etc.

# For some reason there's no _OPTIMIZED to mean "anything not debug"...
SET ( CMAKE_INTERPROCEDURAL_OPTIMIZATION ON )
SET ( CMAKE_INTERPROCEDURAL_OPTIMIZATION_DEBUG OFF )

IF ( WIN32 )

    INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_SOURCE_DIR}/../proj/win )

	ADD_COMPILE_DEFINITIONS( _ITERATOR_DEBUG_LEVEL=$<IF:$<CONFIG:Debug>,2,0> )
    ADD_COMPILE_DEFINITIONS( _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_DEPRECATE )
    ADD_COMPILE_DEFINITIONS(  WIN32 UNICODE _UNICODE )

    ADD_COMPILE_OPTIONS(
    	# Debug info always, enforce for scope, C++ exceptions, precise FP
        "/Zi" "/Zc:forScope" "/EHsc" "/fp:precise"
        # No comdat, always bufsec, string dedup, frame pointers, intrinsics
        "/Gy-" "/GS" "/GF" "/Oy-" "/Oi"
        # Browse inf, no fixed enum warn, no logo, SS2
        "/FR" "/wd4480" "/nologo" "/arch:SSE2"
        # Multi-threaded DLL with or w/o debug
    	$<IF:$<CONFIG:Debug>,"/MTd","/MT">
    	# Inlining level 0 for debug, 2 otherwise
    	$<IF:$<CONFIG:Debug>,"/Ob0","/Ob2">
    	# No opt in Debug, otherwise optimize for size
		$<IF:$<CONFIG:Debug>,"/Od","/O1"> $<$<CONFIG:Debug>,,"/Os">
		# Whole program opt in non-Debug
		$<IF:$<CONFIG:Debug>,,"/GL">
    )
    # For non-Debug links, remove unreference stuff and use LTO
    LINK_LIBRARIES( OPTIMIZED "/OPT:REF" "/LTCG" )

ELSE()

	IF ( NOT APPLE AND ( NOT CMAKE_CXX_COMPILER_ID MATCHES Clang OR CMAKE_CXX_COMPILER_VERSION VERSION_LESS 6.0 ) )
		MESSAGE( FATAL_ERROR "You need Clang6.0 or newer to build on Linux." )
	ENDIF()
	INCLUDE( CheckCXXSourceCompiles )
	CHECK_CXX_SOURCE_COMPILES( "#include <__config>\nint main(){}" LIBCXX_IS_DEFAULT )
	IF ( NOT LIBCXX_IS_DEFAULT )
		ADD_COMPILE_OPTIONS( "-stdlib=libc++" )
		LINK_LIBRARIES( "-stdlib=libc++" )
	ENDIF()

	LINK_LIBRARIES( "-fuse-ld=gold" )

	# Every warning ever, then shut off the ones we don't want
	IF ( CMAKE_BUILD_TYPE MATCHES Debug )
		ADD_COMPILE_OPTIONS( "-Weverything" )
		ADD_COMPILE_OPTIONS( "-Wno-double-promotion" "-Wno-cstring-format-directive" )
		ADD_COMPILE_OPTIONS( "-Wno-packed" "-Wno-pedantic" )
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
	ELSE()
	ENDIF ()

	ADD_COMPILE_OPTIONS( "-m${BUILD_BITNESS}" "-fno-omit-frame-pointer" )
	LINK_LIBRARIES( "-lm" "-ldl" "-lpthread" ) # Not needed on macOS, but harmless anyway

	# Enable Address and Undefined Behavior Sanitizers
	IF ( PN_SANITIZE )
		ADD_COMPILE_OPTIONS( "-fstack-protector-all" "-fsanitize=address" "-fno-common" "-fsanitize=undefined" )
		LINK_LIBRARIES( "-fsanitize=address" "-fno-common" "-fsanitize=undefined" )
	ENDIF()

ENDIF()

ADD_COMPILE_DEFINITIONS( $<IF:$<CONFIG:Debug>,DEBUG,NDEBUG>=1 )

# Let the user specify overriding flags
SET( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${PN_C_FLAGS}" )
SET( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${PN_CXX_FLAGS}" )
