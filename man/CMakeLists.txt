project(segyio-man)

if(NOT UNIX)
    return()
endif()

install(FILES segyio-cath.1
        DESTINATION ${CMAKE_INSTALL_MANDIR}
)
