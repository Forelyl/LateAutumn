project(late_autumn_client VERSION 0.2.0)

add_executable(${PROJECT_NAME}
    client.cpp
    RenderWindow/RenderWindow.cpp                   RenderWindow/RenderWindow.h
    Utils/utils.cpp                                 Utils/utils.h

    Entities/Entity/Entity.cpp                      Entities/Entity/Entity.h
    Entities/Block/Block.cpp                        Entities/Block/Block.h
    Entities/Movable/Movable.cpp                    Entities/Movable/Movable.h
    Entities/Player/Player.cpp                      Entities/Player/Player.h
    Entities/Opponent/Opponent.cpp                  Entities/Opponent/Opponent.h
    Entities/Text/Text.cpp                          Entities/Text/Text.h

    Entities/Character_array/Character_array.cpp    Entities/Character_array/Character_array.h

    Network/Network.cpp                             Network/Network.h


    Map/Map.cpp Map/Map.h

    Game/Game.cpp Game/Game.h
)

target_include_directories(${PROJECT_NAME}
                           PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/RenderWindow
                           PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Map
                           PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Game
                           PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Utils
                           PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Network

                           PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Entities/Entity
                           PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Entities/Block
                           PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Entities/Movable
                           PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Entities/Player
                           PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Entities/Opponent
                           PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Entities/Text
                           PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Entities/Character_array
)


# ----------
# include network

# include custom libraries
target_link_libraries(${PROJECT_NAME} PRIVATE late_autumn_error_repairing)

# include WinSocket
target_link_libraries(${PROJECT_NAME} PRIVATE wsock32 ws2_32)
# ----------

# include SDL 3
include("$ENV{My_CMake_libraries}/Mingw_cmake/SDL3.cmake")
include("$ENV{My_CMake_libraries}/Mingw_cmake/SDL3_image.cmake")
include("$ENV{My_CMake_libraries}/Mingw_cmake/SDL3_ttf.cmake")

find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3-shared)
find_package(SDL3_image REQUIRED)
find_package(SDL3_ttf REQUIRED)


target_include_directories(${PROJECT_NAME}
    PRIVATE ${SDL3_INCLUDE_DIRS}
    PRIVATE ${SDL3_image_INCLUDE_DIRS}
    PRIVATE ${SDL3_ttf_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE SDL3::SDL3
    PRIVATE SDL3_image::SDL3_image
    PRIVATE SDL3_ttf::SDL3_ttf
)

# adding .dll
add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${SDL3_MY_VAR_DLL} ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Copying SDL3.dll to build dir"
)

add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${SDL3_image_MY_VAR_DLL} ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Copying SDL3_image.dll to build dir"
)

add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${SDL3_TTF_MY_VAR_DLL} ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Copying SDL3_tff.dll to build dir"
)

# adding assets
add_custom_command(
    TARGET  ${PROJECT_NAME}  POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/assets ${CMAKE_CURRENT_BINARY_DIR}/assets

)

# adding data
add_custom_command(
    TARGET  ${PROJECT_NAME}  POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/data ${CMAKE_CURRENT_BINARY_DIR}/data

)

# adding custom .dll
file(GLOB TEMP_DLLs "${CMAKE_BINARY_DIR}/Network/*.dll")

foreach(DLL_FILE ${TEMP_DLLs})
    # TODO: why it copying only first time: try delete copied .dll and rebuild
    add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${DLL_FILE} ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Copying Custom utils to build dir of executable"
    )
endforeach()