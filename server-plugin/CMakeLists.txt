cmake_minimum_required(VERSION 2.8.11)
PROJECT(NoCheatZ-4 CXX)

if (${CMAKE_BINARY_DIR} STREQUAL ${PROJECT_SOURCE_DIR})
  message (FATAL_ERROR "Wrong usage. Usage: 'mkdir builddir; cd builddir; cmake ..;make'")
endif (${CMAKE_BINARY_DIR} STREQUAL ${PROJECT_SOURCE_DIR})

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

execute_process(
  COMMAND git describe --tags --always --dirty
  OUTPUT_VARIABLE VERSION_GIT
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
  COMMAND git describe --tags --abbrev=0
  OUTPUT_VARIABLE VERSION_GIT_SHORT
  OUTPUT_STRIP_TRAILING_WHITESPACE
)


ADD_DEFINITIONS(
  -Wall -m32 -std=c++11
  -mtune=i486 -march=x86-64 -mmmx
  -DTIXML_USE_TICPP -Dstricmp=strcasecmp -D_stricmp=strcasecmp -Dstrnicmp=strncasecmp
  -msse -D_LINUX -DLINUX -D_alloca=alloca
  -fPIC -Wno-deprecated -fno-exceptions -fpermissive
  -fno-strict-aliasing
  -Wno-non-virtual-dtor -Wno-overloaded-virtual
  -Wall -Werror
  -DNCZ_CSS
  -DGNUC -DPOSIX -DCOMPILER_GCC
  -DNCZ_VERSION_GIT="${VERSION_GIT}" -DNCZ_VERSION_GIT_SHORT="${VERSION_GIT_SHORT}"
  -I${PROJECT_SOURCE_DIR}/Code
  -I${PROJECT_SOURCE_DIR}/SourceSdk
  -I${PROJECT_SOURCE_DIR}/SourceSdk/Interfaces/Protobuf
  -I${PROJECT_SOURCE_DIR}/SourceSdk/Interfaces/Protobuf/protobuf-2.5.0/src
)

execute_process(COMMAND git submodule update --init)

file(GLOB server-plugin_SRC
  "Code/*.cpp" "Code/*.h"
  "Code/Hooks/*.cpp" "Code/Hooks/*.h"
  "Code/Misc/*.cpp" "Code/Misc/*.h"
  "Code/Players/*.cpp" "Code/Players/*.h"
  "Code/Systems/Blockers/*.cpp" "Code/Systems/Blockers/*.h"
  "Code/Systems/Testers/*.cpp" "Code/Systems/Testers/*.h"
  "Code/Systems/*.cpp" "Code/Systems/*.h"
  "SourceSdk/SourceSdk.cpp" "SourceSdk/SourceSdk.h"
  "SourceSdk/Interfaces/Protobuf/*.cpp" "SourceSdk/Interfaces/Protobuf/*.h"
)
list(REMOVE_ITEM server-plugin_SRC "${PROJECT_SOURCE_DIR}/Code/Misc/StackWalker.cpp")
ADD_LIBRARY(nocheatzX SHARED ${server-plugin_SRC})
set_target_properties(nocheatzX PROPERTIES
  PREFIX ""
)
target_link_libraries(nocheatzX ${CMAKE_DL_LIBS} "${PROJECT_SOURCE_DIR}/SourceSdk/Interfaces/Protobuf/protobuf-2.5.0/src/.libs/libprotobuf.a")
set(CMAKE_SHARED_LINKER_FLAGS "-Wl,--as-needed,--no-undefined -static-libgcc -ldl -lm -m32")
message ("compiling protobuf")
execute_process(
  COMMAND "/bin/sh" "${PROJECT_SOURCE_DIR}/../scripts/make-protobuf.sh" 
  WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}" 
)

message("-------------------------")
message("Version           : ${VERSION_GIT}")
message("System            : ${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_VERSION}")
message("Compiler          : ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
if(CMAKE_BUILD_TYPE MATCHES Debug)
  message("Debug             : yes")
  ADD_DEFINITIONS(-g -ggdb3 -O0 -D_DEBUG -DDEBUG)
else(CMAKE_BUILD_TYPE MATCHES Debug)
  message("Debug             : no (run \"cmake . -DCMAKE_BUILD_TYPE=Debug\")")
  ADD_DEFINITIONS(-DNDEBUG -O3 -funroll-loops -s -pipe)
endif(CMAKE_BUILD_TYPE MATCHES Debug)
message("-------------------------")
