cmake_minimum_required(VERSION 3.13.0 FATAL_ERROR)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

project(lua_cArgParse C CXX)

# Global configuration types
set(CMAKE_CONFIGURATION_TYPES
    "Debug" # cmake -DCMAKE_BUILD_TYPE=Debug
    "Release"
    CACHE STRING "" FORCE
)

# Use solution folders feature
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/build/${CMAKE_SYSTEM_PROCESSOR}/${CMAKE_BUILD_TYPE})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})


project(lua C)
set(FILES
    "../lua/lapi.c"
    "../lua/lapi.h"
    "../lua/lauxlib.c"
    "../lua/lauxlib.h"
    "../lua/lbaselib.c"
    "../lua/lcode.c"
    "../lua/lcode.h"
    "../lua/lcorolib.c"
    "../lua/lctype.c"
    "../lua/lctype.h"
    "../lua/ldblib.c"
    "../lua/ldebug.c"
    "../lua/ldebug.h"
    "../lua/ldo.c"
    "../lua/ldo.h"
    "../lua/ldump.c"
    "../lua/lfunc.c"
    "../lua/lfunc.h"
    "../lua/lgc.c"
    "../lua/lgc.h"
    "../lua/linit.c"
    "../lua/liolib.c"
    "../lua/ljumptab.h"
    "../lua/llex.c"
    "../lua/llex.h"
    "../lua/llimits.h"
    "../lua/lmathlib.c"
    "../lua/lmem.c"
    "../lua/lmem.h"
    "../lua/loadlib.c"
    "../lua/lobject.c"
    "../lua/lobject.h"
    "../lua/lopcodes.c"
    "../lua/lopcodes.h"
    "../lua/lopnames.h"
    "../lua/loslib.c"
    "../lua/lparser.c"
    "../lua/lparser.h"
    "../lua/lprefix.h"
    "../lua/lstate.c"
    "../lua/lstate.h"
    "../lua/lstring.c"
    "../lua/lstring.h"
    "../lua/lstrlib.c"
    "../lua/ltable.c"
    "../lua/ltable.h"
    "../lua/ltablib.c"
    "../lua/ltests.c"
    "../lua/ltests.h"
    "../lua/ltm.c"
    "../lua/ltm.h"
    "../lua/lua.c"
    "../lua/lua.h"
    "../lua/luaconf.h"
    "../lua/lualib.h"
    "../lua/lundump.c"
    "../lua/lundump.h"
    "../lua/lutf8lib.c"
    "../lua/lvm.c"
    "../lua/lvm.h"
    "../lua/lzio.c"
    "../lua/lzio.h"
#    "../lua/onelua.c"
)
add_library(${PROJECT_NAME} STATIC ${FILES})


project(lua_cArgParse CXX)
set(FILES
    "../lua_cArgParse.hpp"
    "../README.md"
    "tests.cpp"
)
add_executable(${PROJECT_NAME} ${FILES})
add_dependencies(${PROJECT_NAME} lua)
target_include_directories(lua_cArgParse PRIVATE ${CMAKE_HOME_DIRECTORY}/../lua)
target_link_libraries(lua_cArgParse lua)
