# Usage: cmake -DSRC=<file> -DDST=<file> -P CopyIfMissing.cmake
if(NOT EXISTS "${DST}" AND EXISTS "${SRC}")
    get_filename_component(DST_DIR "${DST}" DIRECTORY)
    file(MAKE_DIRECTORY "${DST_DIR}")
    configure_file("${SRC}" "${DST}" COPYONLY)
endif()
