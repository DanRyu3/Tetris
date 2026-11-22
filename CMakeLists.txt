cmake_minimum_required(VERSION 3.20)
project(Tetris LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/utf-8 /wd4200)
endif()

# 한 번 받은 Rsact는 매번 GitHub에서 다시 확인하지 않음 (구성 속도 향상)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

# --- Rsact 가져오기 (Rust 빌드는 Corrosion이 처리) ---
include(FetchContent)
FetchContent_Declare(
    rsact
    GIT_REPOSITORY https://github.com/MovingJu/Rsact.git
    GIT_TAG        main
)
FetchContent_MakeAvailable(rsact)

# --- 내 게임: src 폴더의 .cpp를 자동으로 전부 포함 ---
file(GLOB TETRIS_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/*.cpp)

add_executable(tetris ${TETRIS_SOURCES} "src/Menu.h" "src/Menu.cpp" "src/Scene.h")
target_include_directories(tetris PRIVATE src)
target_link_libraries(tetris PRIVATE rsact::rsact_ffi)