# libsol cmake build file
cmake_minimum_required(VERSION 2.6)
project(libsol C)

# build for C99
add_definitions(-std=c99)

if(${UNIX})
    add_definitions(-D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -D_POSIX_C_SOURCE=200809L)
endif()

# load extra modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

# define sources and headers
set (LIBSOL_SOURCES
    runtime.c
    sol.c
    solar.c
    solfunc.c
    sollist.c
    solop.c
    soltoken.c
    soltypes.c
    solerror.c
    solevent.c
    solutils.c)
set (LIBSOL_PUBLIC_HEADERS
    runtime.h
    sol.h
    solar.h
    solfunc.h
    sollist.h
    solop.h
    soltoken.h
    soltypes.h
    solerror.h
    solevent.h
    solutils.h
    uthash.h)
set (LIBSOL_PRIVATE_HEADERS
    )

# define sol library for building
add_library(sol SHARED ${LIBSOL_SOURCES} ${LIBSOL_PUBLIC_HEADERS} ${LIBSOL_PRIVATE_HEADERS})

# load and link libyaml
find_package(LibYAML REQUIRED)
include_directories(${LIBYAML_INCLUDE_DIR})
target_link_libraries(sol ${LIBYAML_LIBRARIES})

# load an link libuv
find_package(Libuv REQUIRED)
include_directories(${LIBUV_INCLUDE_DIRS})
target_link_libraries(sol ${LIBUV_LIBRARIES})

# install libraries
install(TARGETS sol LIBRARY DESTINATION lib)
install(FILES ${LIBSOL_PUBLIC_HEADERS} DESTINATION include/sol)
