project(segyio-man)

if(NOT UNIX)
    return()
endif()

install(FILES segyio-cath.1
              segyio-catb.1
              segyio-catr.1
              segyio-crop.1          
        DESTINATION ${CMAKE_INSTALL_MANDIR}/man1
)
