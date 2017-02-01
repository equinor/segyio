unset(_Julia_NAMES)
list(APPEND _Julia_NAMES julia)
find_program(JULIA_EXECUTABLE NAMES ${_Julia_NAMES})

if(NOT JULIA_EXECUTABLE)
    foreach(_CURRENT_VERSION "0.4 0.5")
        find_program(JULIA_EXECUTABLE
            NAMES julia
            PATHS [HKEY_LOCAL_MACHINE\\SOFTWARE\\Julia\\${_CURRENT_VERSION}\\InstallPath]
            )
    endforeach()
endif()

if(JULIA_EXECUTABLE)
    execute_process(COMMAND
                    "${JULIA_EXECUTABLE}" -e
                    "print(join([VERSION.major VERSION.minor VERSION.patch],';'))"
                    OUTPUT_VARIABLE _VERSION
                    RESULT_VARIABLE _JULIA_VERSION_RESULT)

    if(NOT _JULIA_VERSION_RESULT)
        string(REPLACE ";" "." JULIA_VERSION_STRING "${_VERSION}")
        list(GET _VERSION 0 JULIA_VERSION_MAJOR)
        list(GET _VERSION 1 JULIA_VERSION_MINOR)
        list(GET _VERSION 2 JULIA_VERSION_PATCH)
        if(JULIA_VERSION_PATCH EQUAL 0)
            string(REGEX REPLACE
                   "\\.0$" ""
                   JULIA_VERSION_STRING
                   ${JULIA_VERSION_STRING})
        endif()
    endif()
    unset(_VERSION)
    unset(_JULIA_VERSION_RESULT)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(JuliaInterp
                                  REQUIRED_VARS JULIA_EXECUTABLE
                                  VERSION_VAR JULIA_VERSION_STRING)

mark_as_advanced(JULIA_EXECUTABLE)
