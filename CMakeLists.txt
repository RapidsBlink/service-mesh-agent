cmake_minimum_required(VERSION 3.0)
project(agent C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH}) # To allow CMake to locate our Find*.cmake files
# Options. Turn on with 'cmake -Dmyvarname=ON'.
option(test "Build all tests." ON) # Makes boolean 'test' available.
option(enableUDP "enable UDP between agents" OFF)
option(enableLOG "enable loging" ON)
option(enableUDP_Control "enable Control Channel and mannual gc" OFF)

add_compile_options(-O3 -Wall)

if (enableLOG)
    add_definitions(-DUSE_LOG)
endif ()

if (enableUDP)
    add_definitions(-DUSE_UDP)
endif ()

if (enableUDP_Control)
    add_definitions(-DUSE_UDP_CONTROL)
endif ()

if (enableUDP)
    if(enableUDP_Control)
        message(FATAL_ERROR "cannot use UDP for transmission and control channel at the same time!" )
    endif()
endif()

include(GetGitRevisionDescription)
get_git_head_revision(GIT_REFSPEC GIT_SHA1)
add_definitions("-DGIT_SHA1=\"${GIT_SHA1}\"")

include(CheckCCompilerFlag)
CHECK_C_COMPILER_FLAG("-msse4.2" COMPILER_OPT_SSE42_SUPPORTED)
CHECK_C_COMPILER_FLAG("-mpopcnt" COMPILER_OPT_POPCNT_SUPPORTED)

if (COMPILER_OPT_SSE42_SUPPORTED)
    add_compile_options(-msse4.2)
endif ()
if (COMPILER_OPT_POPCNT_SUPPORTED)
    add_compile_options(-mpopcnt)
endif ()

find_package(Libuv REQUIRED)
if (${LIBUV_FOUND})
    include_directories(${LIBUV_INCLUDE_DIR})
endif ()

find_package(CURL REQUIRED)
include_directories(${CURL_INCLUDE_DIR})

set(CPP_SRC
        src/utils/log.h src/utils/log.c 3rd-deps/base64/b64.h 3rd-deps/base64/decode.c 3rd-deps/base64/encode.c
        3rd-deps/picohttpparser/picohttpparser.h 3rd-deps/picohttpparser/picohttpparser.c src/utils/blockQueue.h
        src/utils/blockQueue.c src/utils/tokenizer.h src/utils/tokenizer.c src/utils/etcd.h
        src/utils/etcd.c 3rd-deps/jsmn/jsmn.h 3rd-deps/jsmn/jsmn.c 3rd-deps/base64/b64.h 3rd-deps/base64/encode.c
        3rd-deps/base64/decode.c src/utils/utils.h src/utils/utils.c src/utils/loadBalancer.h
        src/utils/loadBalancer.c src/utils/atomic_integer.h 3rd-deps/uthash/include/uthash.h src/utils/tokenizer_fast_req.c
        src/config.h src/utils/jvm_cmd.h src/utils/jvm_cmd.c src/utils/http_req_queue.h src/utils/http_req_queue.c)

add_executable(agent ${CPP_SRC} src/main.c src/provider_agent.h src/provider_agent.c src/consumer_agent.h src/consumer_agent.c src/utils/tokenizer_fast_req.c)
target_link_libraries(agent ${LIBUV_LIBRARIES} ${CURL_LIBRARIES})

if (test)
    add_subdirectory(src/utils/tests)
    add_subdirectory(src/playground)
endif ()