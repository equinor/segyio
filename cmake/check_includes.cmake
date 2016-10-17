include(CheckIncludeFile)

# Portability checks; look for htons function
check_include_file("netinet/in.h" HAVE_NETINET_IN_H)
check_include_file("arpa/inet.h" HAVE_ARPA_INET_H)
check_include_file("winsock2.h" HAVE_WINSOCK2_H)

if (HAVE_NETINET_IN_H)
    add_definitions("-DHAVE_NETINET_IN_H")
elseif (HAVE_ARPA_INET_H)
    add_definitions("-DHAVE_ARPA_INET_H")
elseif (HAVE_WINSOCK2_H)
    add_definitions("-DHAVE_WINSOCK2_H")
else()
    message(FATAL_ERROR "Could not find htons.")
endif()


