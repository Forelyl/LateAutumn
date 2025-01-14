project(late_autumn_server VERSION 0.2.0)

add_executable(${PROJECT_NAME}
    server.cpp

    Network/Network.cpp Network/Network.h
    Players/Players.cpp Players/Players.h
)

target_include_directories(${PROJECT_NAME}
                           PRIVATE
                           ${CMAKE_CURRENT_SOURCE_DIR}/Network
                           ${CMAKE_CURRENT_SOURCE_DIR}/Players
)

# include custom libraries
target_link_libraries(${PROJECT_NAME} PRIVATE late_autumn_error_repairing)

# include WinSocket
target_link_libraries(${PROJECT_NAME} PRIVATE wsock32 ws2_32)

# include SDL 3
include("$ENV{My_CMake_libraries}/Mingw_cmake/SDL3.cmake")
find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3-shared)


target_include_directories(${PROJECT_NAME}
    PRIVATE ${SDL3_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE SDL3::SDL3
)

# adding .dll
add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${SDL3_MY_VAR_DLL} ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Copying SDL3.dll to build dir"
)

file(GLOB TEMP_DLLs "${CMAKE_BINARY_DIR}/Network/*.dll")

foreach(DLL_FILE ${TEMP_DLLs})
    # TODO: why it copying only first time: try delete copied .dll and rebuild
    add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${DLL_FILE} ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Copying Custom utils to build dir of executable"
    )
endforeach()

# adding data
add_custom_command(
    TARGET  ${PROJECT_NAME}  POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/data ${CMAKE_CURRENT_BINARY_DIR}/data

)