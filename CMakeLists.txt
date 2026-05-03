cmake_minimum_required(VERSION 3.15)

project(mediapumpd C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# tags 생성 활성화(e.g. ctags)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB SRC_FILES ${PROJECT_SOURCE_DIR}/src/*.c)
add_executable(${PROJECT_NAME} ${SRC_FILES})

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build" FORCE)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE}"
)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    # -flto (Link Time Optimization)
    set_target_properties(mediapumpd PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
    # 메모리 오류 탐지 설정
    target_compile_options(mediapumpd PRIVATE -fsanitize=address -g)
    target_link_options(mediapumpd PRIVATE -fsanitize=address)
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE _GNU_SOURCE)

target_include_directories(${PROJECT_NAME}
    PRIVATE ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(${PROJECT_NAME} PRIVATE uring)

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
#        liburing-dev
# )
