#  Copyright (C)  2016 Statoil ASA, Norway.
#
#  This file is part of ERT - Ensemble based Reservoir Tool.
#
#  ERT is free software: you can redistribute it and/or modify it under the
#  terms of the GNU General Public License as published by the Free Software
#  Foundation, either version 3 of the License, or (at your option) any later
#  version.
#
#  ERT is distributed in the hope that it will be useful, but WITHOUT ANY
#  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
#  A PARTICULAR PURPOSE.
#
#  See the GNU General Public License at <http://www.gnu.org/licenses/gpl.html>
#  for more details



# The basic assumption of this package is PEP 396 -- Module Version Numbers as
# layed out in https://www.python.org/dev/peps/pep-0396/

# Unfortunately, not all Python modules expose a version number, like inspect.
# Other Python modules expose several version numbers, e.g. one for the
# underlying software and one for the python packaging, like SQLite and PyQt.

cmake_minimum_required (VERSION 2.8.1)



# try import python module, if success, check its version, store as PY_module.
# the module is imported as-is, hence the case (e.g. PyQt4) must be correct.
#
# if given a second argument, the accessor, we call accessor on the module
# instead of the default __version__.
#
# (Yes, accessor could potentially be a function like "os.delete_everything()".)
macro(python_module_version module)
  set(PY_VERSION_ACCESSOR "__version__")
  set(PY_module_name ${module})

  if(${PY_module_name} STREQUAL "PyQt4")
    set(PY_module_name "PyQt4.Qt")
  endif()
  if(${PY_module_name} STREQUAL "PyQt4.Qt")
    set(PY_VERSION_ACCESSOR "PYQT_VERSION_STR")
  endif()

  if(${PY_module_name} STREQUAL "serial")
    set(PY_VERSION_ACCESSOR "VERSION")
  endif()

  if(${PY_module_name} STREQUAL "sqlite")
    set(PY_VERSION_ACCESSOR "version")
  endif()


  #  ARGUMENTS: module accessor
  set (extra_macro_args ${ARGN})
  list(LENGTH extra_macro_args num_extra_args)
  if (${num_extra_args} GREATER 0)
    list(GET extra_macro_args 0 accessor)
    set(PY_VERSION_ACCESSOR ${accessor})
  endif()

  execute_process(COMMAND "${PYTHON_EXECUTABLE}" "-c"
    "# set module's version to py_mv and print it
import ${PY_module_name} as py_m
py_mv = '0.0.0' # output if no accessor is found
if hasattr(py_m, '${PY_VERSION_ACCESSOR}'):
    py_mv = py_m.${PY_VERSION_ACCESSOR}
print(py_mv)
"
    RESULT_VARIABLE _${module}_fail    # error code 0 if module is importable
    OUTPUT_VARIABLE _${module}_version # module.accessor or "0.0.0" if no such
    ERROR_VARIABLE stderr_output
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(NOT _${module}_fail)
    set(PY_${module} ${_${module}_version})
  endif()

  # clean up
  unset(PY_VERSION_ACCESSOR)
  unset(PY_module_name)
  unset(extra_macro_args)
endmacro()



# If we find the correct module and new enough version, set PY_package, where
# "package" is the given argument to the version we found else, display warning
# and do not set any variables.
macro(python_module package)

  #  ARGUMENTS: package package_req module_version version_req accessor
  set (extra_macro_args ${ARGN})
  # Did we get any optional args?
  list(LENGTH extra_macro_args num_extra_args)
  if (${num_extra_args} GREATER 0)
    list(GET extra_macro_args 0 package_req)
  else()
    set(package_req "REQUIRED") # requirement not set, is required
  endif ()
  if (${num_extra_args} GREATER 1)
    list(GET extra_macro_args 1 module_version)
  else()
    set(module_version "0.0.0") # module_version not set, 0.0.0 is ok
  endif ()
  if (${num_extra_args} GREATER 2)
    list(GET extra_macro_args 2 version_req)
  else()
    set(version_req "MINIMUM") # version requirement not set, is minimum
  endif ()
  if (${num_extra_args} GREATER 3)
    list(GET extra_macro_args 3 accessor)
  endif ()

  # Setting warning/error output level
  set(PY_MSG_ERR  SEND_ERROR)
  set(PY_MSG_WARN WARNING)
  if(${package_req} STREQUAL "QUIET")
    set(PY_MSG_ERR  STATUS)
    set(PY_MSG_WARN STATUS)
  endif()

  # We are done expanding the optional arguments

  python_module_version(${package} ${accessor})

  # package not found in system
  if(NOT DEFINED PY_${package})
    if(${package_req} STREQUAL "OPTIONAL")
      message(${PY_MSG_WARN} "Could not find Python module " ${package})
    else()
      message(${PY_MSG_ERR} "Could not find Python module " ${package})
    endif()

  else()
    # package found in system

    if (${version_req} STREQUAL "EXACT" AND NOT ${PY_${package}} VERSION_EQUAL ${module_version})
      message(${PY_MSG_ERR} "Python module ${package} not exact.  "
        "Wanted EXACT ${module_version}, found ${PY_${package}}")
    elseif (${version_req} STREQUAL "OPTIONAL" AND ${PY_${package}} VERSION_LESS ${module_version})
      message(${PY_MSG_WARN} "Python module ${package} too old.  "
        "Wanted ${module_version}, found ${PY_${package}}")
    elseif (${version_req} STREQUAL "MINIMUM" AND ${PY_${package}} VERSION_LESS ${module_version})
      message(${PY_MSG_ERR} "Python module ${package} too old.  "
        "Wanted MINIMUM ${module_version}, found ${PY_${package}}")
    else()
      if(NOT DEFINED accessor)
        message(STATUS "Found ${package}.  "
                "${PY_${package}} >= ${module_version}")
      else()
        message(STATUS "Found ${package}.  "
                "${PY_${package}} >= ${module_version} (" ${accessor} ")")
      endif()
    endif()
  endif()

  # clean up
  unset(package_req)
  unset(module_version)
  unset(version_req)
  unset(accessor)
  unset(extra_macro_args)
  set(PY_MSG_ERR)
  set(PY_MSG_WARN)
endmacro()
