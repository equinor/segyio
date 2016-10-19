if (NOT MSVC)
    # assuming gcc-style options
    set(C_WARNINGS "-Wall -Wextra -pedantic -Wformat-nonliteral -Wcast-align -Wpointer-arith -Wmissing-declarations -Wcast-qual -Wshadow -Wwrite-strings -Wchar-subscripts -Wredundant-decls")
    set(CMAKE_C_FLAGS_DEBUG "${C_WARNINGS} ${CMAKE_C_FLAGS_DEBUG}")
    set(CMAKE_C_FLAGS_RELWITHDEBINFO "${C_WARNINGS} ${CMAKE_C_FLAGS_RELWITHDEBINFO}")
else() 
    add_definitions("/W3 /wd4996")
endif ()

