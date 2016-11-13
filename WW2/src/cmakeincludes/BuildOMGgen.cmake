# Define OMGgen itself
ADD_SUBDIRECTORY(omggen)

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
