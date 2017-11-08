# This module looks for mex, the MATLAB compiler.
# The following variables are defined when the script completes:
#   MATLAB_MEX: location of mex compiler
#   MATLAB_ROOT: root of MATLAB installation
#   MATLABMEX_FOUND: 0 if not found, 1 if found

SET(MATLABMEX_FOUND 0)
SET(MATLABMCC_FOUND 0)

IF(WIN32)
  # Win32 is Untested
  # Taken from the older FindMatlab.cmake script as well as
  # the modifications by Ramon Casero and Tom Doel for Gerardus.

  # Search for a version of Matlab available, starting from the most modern one
  # to older versions.
  FOREACH(MATVER "7.20" "7.19" "7.18" "7.17" "7.16" "7.15" "7.14" "7.13" "7.12" "7.11" "7.10" "7.9" "7.8" "7.7" "7.6" "7.5" "7.4")
    IF((NOT DEFINED MATLAB_ROOT)
        OR ("${MATLAB_ROOT}" STREQUAL "")
        OR ("${MATLAB_ROOT}" STREQUAL "/registry"))
      GET_FILENAME_COMPONENT(MATLAB_ROOT
        "[HKEY_LOCAL_MACHINE\\SOFTWARE\\MathWorks\\MATLAB\\${MATVER};MATLABROOT]"
        ABSOLUTE)
      SET(MATLAB_VERSION ${MATVER})
    ENDIF((NOT DEFINED MATLAB_ROOT)
      OR ("${MATLAB_ROOT}" STREQUAL "")
      OR ("${MATLAB_ROOT}" STREQUAL "/registry"))
  ENDFOREACH(MATVER)

  FIND_PROGRAM(MATLAB_MEX
          mex
          ${MATLAB_ROOT}/bin
          )
  FIND_PROGRAM(MATLAB_MCC
          mex
          ${MATLAB_ROOT}/bin
          )
ELSE(WIN32)
  # Check if this is a Mac.
  IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    # Mac is untested
    # Taken from the older FindMatlab.cmake script as
    # well as the modifications by Ramon Casero and Tom Doel for Gerardus.

   SET(LIBRARY_EXTENSION .dylib)

    # If this is a Mac and the attempts to find MATLAB_ROOT have so far failed,~
    # we look in the applications folder
    IF((NOT DEFINED MATLAB_ROOT) OR ("${MATLAB_ROOT}" STREQUAL ""))

    # Search for a version of Matlab available, starting from the most modern
    # one to older versions
      FOREACH(MATVER "2016b" "2014b" "R2013b" "R2013a" "R2012b" "R2012a" "R2011b" "R2011a" "R2010b" "R2010a" "R2009b" "R2009a" "R2008b")
        IF((NOT DEFINED MATLAB_ROOT) OR ("${MATLAB_ROOT}" STREQUAL ""))
          IF(EXISTS /Applications/MATLAB_${MATVER}.app)
            SET(MATLAB_ROOT /Applications/MATLAB_${MATVER}.app)

          ENDIF(EXISTS /Applications/MATLAB_${MATVER}.app)
        ENDIF((NOT DEFINED MATLAB_ROOT) OR ("${MATLAB_ROOT}" STREQUAL ""))
      ENDFOREACH(MATVER)

    ENDIF((NOT DEFINED MATLAB_ROOT) OR ("${MATLAB_ROOT}" STREQUAL ""))

    FIND_PROGRAM(MATLAB_MEX
            mex
            PATHS
            ${MATLAB_ROOT}/bin
            )
    FIND_PROGRAM(MATLAB_MCC
            mcc
            PATHS
            ${MATLAB_ROOT}/bin
            )

  ELSE(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    # On a Linux system.  The goal is to find MATLAB_ROOT.
    SET(LIBRARY_EXTENSION .so)

    FIND_PROGRAM(MATLAB_MEX
            mex
            PATHS
            ${MATLAB_ROOT}/bin
            /prog/matlab/R2016B/bin # Statoil location
            /prog/matlab/R2014B/bin # Statoil location
            /opt/matlab/bin
            /usr/local/matlab/bin
            $ENV{HOME}/matlab/bin
            # Now all the versions
            /opt/matlab/[rR]20[0-9][0-9][abAB]/bin
            /usr/local/matlab/[rR]20[0-9][0-9][abAB]/bin
            /opt/matlab-[rR]20[0-9][0-9][abAB]/bin
            /opt/matlab_[rR]20[0-9][0-9][abAB]/bin
            /usr/local/matlab-[rR]20[0-9][0-9][abAB]/bin
            /usr/local/matlab_[rR]20[0-9][0-9][abAB]/bin
            $ENV{HOME}/matlab/[rR]20[0-9][0-9][abAB]/bin
            $ENV{HOME}/matlab-[rR]20[0-9][0-9][abAB]/bin
            $ENV{HOME}/matlab_[rR]20[0-9][0-9][abAB]/bin
            )

    GET_FILENAME_COMPONENT(MATLAB_MEX "${MATLAB_MEX}" REALPATH)
    GET_FILENAME_COMPONENT(MATLAB_BIN_ROOT "${MATLAB_MEX}" PATH)
    # Strip ./bin/.
    GET_FILENAME_COMPONENT(MATLAB_ROOT "${MATLAB_BIN_ROOT}" PATH)

    FIND_PROGRAM(MATLAB_MCC
            mcc
            PATHS
            ${MATLAB_ROOT}/bin
            )

    FIND_PROGRAM(MATLAB_MEXEXT
            mexext
            PATHS
            ${MATLAB_ROOT}/bin
            )

    GET_FILENAME_COMPONENT(MATLAB_MCC "${MATLAB_MCC}" REALPATH)
    GET_FILENAME_COMPONENT(MATLAB_MEXEXT "${MATLAB_MEXEXT}" REALPATH)
  ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
ENDIF(WIN32)

IF(NOT EXISTS "${MATLAB_MEX}" AND "${MatlabMex_FIND_REQUIRED}")
  MESSAGE(FATAL_ERROR "Could not find MATLAB mex compiler; try specifying MATLAB_ROOT.")
ELSE(NOT EXISTS "${MATLAB_MEX}" AND "${MatlabMex_FIND_REQUIRED}")
  IF(EXISTS "${MATLAB_MEX}")
    MESSAGE(STATUS "Found MATLAB mex compiler: ${MATLAB_MEX}")
    MESSAGE(STATUS "MATLAB root: ${MATLAB_ROOT}")
    SET(MATLABMEX_FOUND 1)
  ENDIF(EXISTS "${MATLAB_MEX}")
ENDIF(NOT EXISTS "${MATLAB_MEX}" AND "${MatlabMex_FIND_REQUIRED}")

IF(NOT EXISTS "${MATLAB_MCC}" AND "${MatlabMcc_FIND_REQUIRED}")
  MESSAGE(FATAL_ERROR "Could not find MATLAB mcc compiler; try specifying MATLAB_ROOT.")
ELSE(NOT EXISTS "${MATLAB_MCC}" AND "${MatlabMcc_FIND_REQUIRED}")
  IF(EXISTS "${MATLAB_MCC}")
    MESSAGE(STATUS "Found MATLAB mcc compiler: ${MATLAB_MCC}")
    SET(MATLABMCC_FOUND 1)
  ENDIF(EXISTS "${MATLAB_MCC}")
ENDIF(NOT EXISTS "${MATLAB_MCC}" AND "${MatlabMcc_FIND_REQUIRED}")

MARK_AS_ADVANCED(
  MATLABMEX_FOUND
)

SET(MATLAB_ROOT ${MATLAB_ROOT} CACHE FILEPATH "Path to a matlab installation")

EXECUTE_PROCESS(COMMAND ${MATLAB_MEXEXT} OUTPUT_VARIABLE MEXEXT OUTPUT_STRIP_TRAILING_WHITESPACE)

macro(mexo MEX_OBJECT)
    set(MEX_CFLAGS -fPIC  -std=c99 -Werror)
    set(MEX_LDFLAGS)

    get_property(dirs DIRECTORY . PROPERTY INCLUDE_DIRECTORIES)
    foreach(dir ${dirs})
        set(MEX_CFLAGS ${MEX_CFLAGS} -I${dir})
    endforeach()

    set(MEX_LDFLAGS -shared)

    set(SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/${MEX_OBJECT}.c)
    set(HEADER ${CMAKE_CURRENT_SOURCE_DIR}/${MEX_OBJECT}.h)
    set(OBJECT ${CMAKE_CURRENT_BINARY_DIR}/${MEX_OBJECT}.o)

    add_custom_command(OUTPUT ${OBJECT}
            COMMAND
            ${MATLAB_MEX}
            -c
            CC="${CMAKE_C_COMPILER}"
            LD="${CMAKE_CXX_COMPILER}"
            CFLAGS="${MEX_CFLAGS}"
            LDFLAGS="${MEX_LDFLAGS}"
            -outdir ${CMAKE_CURRENT_BINARY_DIR}
            ${SOURCE}
            DEPENDS
            ${SOURCE}
            ${HEADER}
            )

    add_custom_target(${MEX_OBJECT} ALL DEPENDS ${OBJECT})
endmacro()

macro(mex MEX_NAME )
    set(DEP ${ARG2})
    set(MEX_CFLAGS -fPIC  -std=c99 -Werror)
    set(MEX_LDFLAGS)

    get_property(dirs DIRECTORY . PROPERTY INCLUDE_DIRECTORIES)
    foreach(dir ${dirs})
        set(MEX_CFLAGS ${MEX_CFLAGS} -I${dir})
    endforeach()

    set(MEX_LDFLAGS -shared)

    set(MEX_SOURCE_FILE ${CMAKE_CURRENT_SOURCE_DIR}/${MEX_NAME}.c)
    set(MEX_RESULT_FILE ${CMAKE_CURRENT_BINARY_DIR}/${MEX_NAME}.${MEXEXT})
    add_custom_command(OUTPUT ${MEX_RESULT_FILE}
            COMMAND
            ${MATLAB_MEX}
            CC="${CMAKE_C_COMPILER}"
            CXX="${CMAKE_CXX_COMPILER}"
            LD="${CMAKE_CXX_COMPILER}"
            CFLAGS="${MEX_CFLAGS}"
            LDFLAGS="${MEX_LDFLAGS}"
            ${OBJECT}
            $<TARGET_FILE:segyio>
            -outdir ${CMAKE_CURRENT_BINARY_DIR}
            ${MEX_SOURCE_FILE}
            DEPENDS
            ${MEX_SOURCE_FILE}
            segyio
            segyutil.o
            )

    add_custom_target(${MEX_NAME} ALL DEPENDS ${MEX_RESULT_FILE} segyutil.o)

    set(${MEX_NAME}_TARGET ${MEX_NAME} PARENT_SCOPE)
    set(${MEX_NAME}_FILE ${MEX_RESULT_FILE} PARENT_SCOPE)

endmacro()


# this isn't meant to be run directly; use matlab_add_test or
# matlab_add_example instead
function(matlab_test TYPE TESTNAME MCC_SOURCE_FILE)
    configure_file(${MCC_SOURCE_FILE} ${MCC_SOURCE_FILE} COPYONLY)

    add_test(NAME ${TESTNAME}
            COMMAND ${MATLAB_ROOT}/bin/matlab -nodisplay -nosplash -nodesktop -r
            "addpath('../mex'), try, run('${MCC_SOURCE_FILE}'), exit(0), catch, exit(-1), end;" < /dev/null
            )
endfunction()

function(add_matlab_test TESTNAME MCC_SOURCE_FILE)
    matlab_test(tests ${TESTNAME} ${MCC_SOURCE_FILE})
endfunction()

# add_matlab_example takes an arbitrary number of arguments which it will
# forward to the example program
function(add_matlab_example TESTNAME MCC_SOURCE_FILE)
    matlab_test(examples ${TESTNAME} ${MCC_SOURCE_FILE})
endfunction()
