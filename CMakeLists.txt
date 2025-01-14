cmake_minimum_required(VERSION 3.30)
project(late_autumn VERSION 0.2.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Util libraries
add_subdirectory(Network)

# Client.cmake
add_subdirectory("Client")

# Server.cmake
add_subdirectory("Server")

# ------------------------------
return()
# Temp file
add_executable(temp temp.cpp)
target_link_libraries(temp PRIVATE wsock32 ws2_32)
target_link_libraries(temp PRIVATE late_autumn_error_repairing)

# add .dll
file(GLOB TEMP_DLLs "${CMAKE_BINARY_DIR}/Network/*.dll")

foreach(DLL_FILE ${TEMP_DLLs})
    # TODO: why it copying only first time: try delete copied .dll and rebuild
    add_custom_command(
        TARGET temp POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${DLL_FILE} ${CMAKE_BINARY_DIR}
        COMMENT "Copying Custom utils to build dir of temp executable"
    )
endforeach()
# ------------------------------


# include Mingw
include("$ENV{My_CMake_compiler}/MinGW.cmake")

# include config of clangd
include ("$ENV{My_CMake_configs}/clangd.cmake")
