cmake_minimum_required(VERSION 3.18)

if(MSVC_VERSION LESS 1910)
	message(FATAL_ERROR "This project require C++17")
endif()

project(argos_proj VERSION "0.0.1")
configure_file(src/common.h.in ${argos_proj_SOURCE_DIR}/src/common.h)

add_executable(argos
src/main.cpp src/solo.cpp

src/common.h src/solo.h src/point.h)

target_compile_features(argos PUBLIC cxx_std_17)

option (ARGOS_PVS "Run static analyze wit PVS-Studio" OFF)
if (ARGOS_PVS)
    include(FetchContent)
    FetchContent_Declare(PVS_CMakeModule
        GIT_REPOSITORY "https://github.com/viva64/pvs-studio-cmake-module.git"
        GIT_TAG        "master")
    FetchContent_MakeAvailable(PVS_CMakeModule)
    include("${pvs_cmakemodule_SOURCE_DIR}/PVS-Studio.cmake")
    
    pvs_studio_add_target(TARGET argos.pvs ALL OUTPUT FORMAT tasklist
        ANALYZE argos MODE 'GA:1,2,3;OP:1' LOG ../PVS_LOG )
endif()
# Add compiler-dependeded options

if(CMAKE_COMPILER_IS_GNUCC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Werror")
endif()
