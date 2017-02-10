include(CheckIncludeFile)
include(CheckFunctionExists)

# Portability checks; look for htons function
check_include_file("netinet/in.h" HAVE_NETINET_IN_H)
check_include_file("arpa/inet.h" HAVE_ARPA_INET_H)
check_include_file("winsock2.h" HAVE_WINSOCK2_H)

if (HAVE_NETINET_IN_H)
    add_definitions("-DHAVE_NETINET_IN_H")
elseif (HAVE_ARPA_INET_H)
    add_definitions("-DHAVE_ARPA_INET_H")
elseif (HAVE_WINSOCK2_H)
    list(APPEND SEGYIO_LIBRARIES ws2_32)
    add_definitions("-DHAVE_WINSOCK2_H")
else()
    message(FATAL_ERROR "Could not find htons.")
endif()

check_include_file("sys/mman.h" HAVE_SYS_MMAN_H)
if (HAVE_SYS_MMAN_H)
    add_definitions("-DHAVE_MMAP")
endif()

check_include_file("sys/stat.h" HAVE_SYS_STAT_H)
if (HAVE_SYS_STAT_H)
    add_definitions("-DHAVE_SYS_STAT_H")

    check_function_exists(_fstati64 HAVE_FSTATI64)
    if (HAVE_FSTATI64)
        add_definitions("-DHAVE_FSTATI64")
    endif ()
endif()
