cmake_minimum_required(VERSION 3.18)

if(MSVC_VERSION LESS 1910)
	message(FATAL_ERROR "This project require C++17")
endif()

project(argos_proj VERSION "0.2.2")
# Запись данных из Git
if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
    execute_process(
        COMMAND git log -1 --format=%h
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        OUTPUT_VARIABLE ARGOS_GIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(
        COMMAND git branch --show-current
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        OUTPUT_VARIABLE ARGOS_GIT_BRANCH
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    string(CONCAT ARGOS_GIT_DATA "git: " ${ARGOS_GIT_BRANCH} "/" ${ARGOS_GIT_HASH})
else()
    set(ARGOS_GIT_BRANCH "main")
    set(ARGOS_GIT_DATA "git: unknown")
endif()

configure_file(src/settings.h.in ${argos_proj_SOURCE_DIR}/src/settings.h)

add_executable(argos
src/main.cpp src/solo.cpp src/io.cpp src/net.cpp
src/network/net_int.cpp src/settings.cpp

src/settings.h src/base.h src/th_queue.h src/io.h
src/net.h src/network/net_def.h src/network/net_int.h)

target_compile_features(argos PUBLIC cxx_std_17)

# Сеть для Windows требует WinSock
if(WIN32)
    target_link_libraries(argos Ws2_32.lib)
endif()

option (ARGOS_PVS "Run static analyze with PVS-Studio" OFF)
if (ARGOS_PVS)
    include(FetchContent)
    FetchContent_Declare(PVS_CMakeModule
        GIT_REPOSITORY "https://github.com/viva64/pvs-studio-cmake-module.git"
        GIT_TAG        "master")
    FetchContent_MakeAvailable(PVS_CMakeModule)
    include("${pvs_cmakemodule_SOURCE_DIR}/PVS-Studio.cmake")
    
    pvs_studio_add_target(TARGET analyze ALL 
                          OUTPUT 
                          FORMAT tasklist
                          ANALYZE argos
                          MODE GA:1,2,3;OP:1,2,3
                          LOG ../PVS_LOG )
endif()

if(CMAKE_COMPILER_IS_GNUCC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Werror")
endif()
