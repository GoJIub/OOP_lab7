cmake_minimum_required(VERSION 3.10)
project(homework6)

# === Настройки C++ ===
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# === Флаги компиляции ===
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror=maybe-uninitialized")

# === Google Test через FetchContent ===
include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.15.0
    TLS_VERIFY false
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# === Включение директорий ===
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# === Основная программа ===
add_executable(${CMAKE_PROJECT_NAME}
    main.cpp
    src/npc.cpp
    src/orc.cpp
    src/squirrel.cpp
    src/bear.cpp
    src/game_utils.cpp
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# === Тесты ===
enable_testing()

add_executable(tests
    test/tests.cpp
    src/npc.cpp
    src/orc.cpp
    src/squirrel.cpp
    src/bear.cpp
    src/game_utils.cpp
)

target_include_directories(tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(tests
    gtest
    gtest_main
    pthread
)

# === Добавление тестов в ctest ===
add_test(NAME Homework5Tests COMMAND tests)
