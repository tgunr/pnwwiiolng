# Define OMGgen itself
ADD_SUBDIRECTORY(omggen)

# Need to create the destination directory
FILE( MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/omg )

# Macro to generate an OMG build command
MACRO( ADD_OMG_TARGET Name )
    SET( OmgPrefix "${CMAKE_CURRENT_SOURCE_DIR}/${Name}" )
    GET_FILENAME_COMPONENT( OmgPrefix ${OmgPrefix} ABSOLUTE )
    GET_FILENAME_COMPONENT( OmgDir ${OmgPrefix}.omg DIRECTORY )
    GET_FILENAME_COMPONENT( OmgFile ${OmgPrefix} NAME )
    ADD_CUSTOM_COMMAND(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/omg/${OmgFile}.cxx ${CMAKE_CURRENT_BINARY_DIR}/omg/${OmgFile}.hxx
        COMMAND OMGgen -D${OmgDir} --cxx=${CMAKE_CURRENT_BINARY_DIR}/omg/${OmgFile}.cxx --hxx=${CMAKE_CURRENT_BINARY_DIR}/omg/${OmgFile}.hxx ${OmgFile}.omg
        MAIN_DEPENDENCY ${OmgPrefix}.omg
        DEPENDS OMGgen ${OmgDir}/typedefs.omg
        WORKING_DIRECTORY ${OmgDir}
        COMMENT "[OMG] ${OmgFile}.omg"
        VERBATIM
    )
ENDMACRO()
