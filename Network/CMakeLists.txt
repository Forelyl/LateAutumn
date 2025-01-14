project(late_autumn_error_repairing VERSION 0.2.0)

add_library(${PROJECT_NAME} SHARED
    error_repairing.cpp
    error_repairing.h
)


target_include_directories(${PROJECT_NAME}
                           INTERFACE
                           ${CMAKE_CURRENT_SOURCE_DIR}
)