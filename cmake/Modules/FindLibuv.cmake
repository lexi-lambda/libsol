# CMake module to search for the libuv library
# 
# If it's found it sets LIBUV_FOUND to TRUE
# and the following variables are set:
#   LIBUV_INCLUDE_DIRS
#   LIBUV_LIBRARIES


find_path(LIBUV_INCLUDE_DIRS NAMES uv.h)
find_library(LIBUV_LIBRARIES NAMES uv libuv)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Libuv DEFAULT_MSG LIBUV_INCLUDE_DIRS LIBUV_LIBRARIES)
