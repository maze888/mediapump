cmake_minimum_required(VERSION 3.15)

project(mediapumpd C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# set(CMAKE_BUILD_TYPE Release)
set(CMAKE_BUILD_TYPE Debug)

# 메모리 오류 탐지
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-fsanitize=address -g)
    add_link_options(-fsanitize=address)
endif()

# 컴파일 명령 DB 생성 활성화
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin/Debug)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/Release)

file(GLOB SRC_FILES ${PROJECT_SOURCE_DIR}/src/*.c)
add_executable(${PROJECT_NAME} ${SRC_FILES})

target_include_directories(${PROJECT_NAME}
    PRIVATE ${PROJECT_SOURCE_DIR}/include
)

# not yet
# link_directories(${PROJECT_SOURCE_DIR}/libs)

# target_link_libraries(${PROJECT_NAME}
#     PRIVATE
#        avformat
#        avcodec
#        avutil
#        swresample
#        m
#        rabbitmq
#        yyjson
#        curl
#        uuid
# )
