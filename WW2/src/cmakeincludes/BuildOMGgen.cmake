# Paths
SET( OMGGEN_INPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/omggen" )
SET( OMGGEN_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/omggen" )

# Make sure the omggen output dir exists, 'cause it doesn't in a fresh build
FILE( MAKE_DIRECTORY ${OMGGEN_OUTPUT_DIR} )

# Pull in the Bison parser and Flex lexer used by OMGgen
BISON_TARGET( OMGParser
    "${OMGGEN_INPUT_DIR}/omggen.y" "${OMGGEN_OUTPUT_DIR}/omggen.tab.cpp"
#    COMPILE_FLAGS "--report=all"
    DEFINES_FILE "${OMGGEN_OUTPUT_DIR}/omggen.tab.h"
)
FLEX_TARGET( OMGLexer
    "${OMGGEN_INPUT_DIR}/omggen.l" "${OMGGEN_OUTPUT_DIR}/omggen.yy.cpp"
    COMPILE_FLAGS "-B --nounistd"
    DEFINES_FILE "${OMGGEN_OUTPUT_DIR}/omggen.yy.h"
)
ADD_FLEX_BISON_DEPENDENCY( OMGLexer OMGParser )

# Define OMGgen itself
ADD_EXECUTABLE( OMGgen
    ${OMGGEN_OUTPUT_DIR}/omggen.tab.cpp ${OMGGEN_OUTPUT_DIR}/omggen.tab.h
    ${OMGGEN_OUTPUT_DIR}/omggen.yy.cpp  ${OMGGEN_OUTPUT_DIR}/omggen.yy.h
    ${OMGGEN_INPUT_DIR}/omggen.cpp     ${OMGGEN_INPUT_DIR}/omggen.h
)
TARGET_INCLUDE_DIRECTORIES( OMGgen PRIVATE "${OMGGEN_INPUT_DIR}" PRIVATE "${OMGGEN_OUTPUT_DIR}" )
SET_PROPERTY(TARGET OMGgen PROPERTY FOLDER "Tools")

# Macro to generate an OMG build command
MACRO( ADD_OMG_TARGET Name )
    SET( OmgPrefix "${CMAKE_CURRENT_SOURCE_DIR}/${Name}" )
    GET_FILENAME_COMPONENT( OmgPrefix ${OmgPrefix} ABSOLUTE )
    GET_FILENAME_COMPONENT( OmgDir ${OmgPrefix}.omg DIRECTORY )
    GET_FILENAME_COMPONENT( OmgFile ${OmgPrefix} NAME )
    ADD_CUSTOM_COMMAND(
        OUTPUT ${OmgPrefix}.cxx ${OmgPrefix}.hxx
        COMMAND OMGgen -D${OmgDir} --cxx=${OmgFile}.cxx --hxx=${OmgFile}.hxx ${OmgFile}.omg
        MAIN_DEPENDENCY ${OmgPrefix}.omg
        DEPENDS OMGgen ${OmgDir}/typedefs.omg
        WORKING_DIRECTORY ${OmgDir}
        COMMENT "[OMG] ${OmgFile}.omg"
        VERBATIM
    )
ENDMACRO()
