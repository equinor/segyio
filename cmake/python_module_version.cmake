# try import python module, if success, check its version, store as PY_module.
# the module is imported as-is, hence the case (e.g. PyQt4) must be correct.
function(python_module_version module)
    set(PY_VERSION_ACCESSOR "__version__")
    set(PY_module_name ${module})

    if(${module} MATCHES "PyQt4")
        set(PY_module_name "PyQt4.Qt")
        set(PY_VERSION_ACCESSOR "PYQT_VERSION_STR")
    endif()

    execute_process(COMMAND "${PYTHON_EXECUTABLE}" "-c" "import ${PY_module_name} as py_m; print(py_m.${PY_VERSION_ACCESSOR})"
            RESULT_VARIABLE _${module}_fail#    error code 0 if success
            OUTPUT_VARIABLE _${module}_version# major.minor.patch
            ERROR_VARIABLE stderr_output
            OUTPUT_STRIP_TRAILING_WHITESPACE)

    if(NOT _${module}_fail)
        set(PY_${module} ${_${module}_version})# local scope, for message
        set(PY_${module} ${_${module}_version} PARENT_SCOPE)
    endif()
endfunction()


# If we find the correct module and new enough version, set PY_package, where
# "package" is the given argument to the version we found else, display warning
# and do not set any variables.
function(python_module package version)
    python_module_version(${package})

    if(NOT DEFINED PY_${package})
        message("Could not find Python module " ${package})
    elseif(${PY_${package}} VERSION_LESS ${version})
        message(WARNING "Python module ${package} too old.  "
                "Wanted ${version}, found ${PY_${package}}")
    else()
        message(STATUS "Found ${package}.  ${PY_${package}} >= ${version}")
        set(PY_${package} ${version} PARENT_SCOPE)
    endif()
endfunction()