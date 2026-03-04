cmake_minimum_required(VERSION 3.16)
project(oeaf C)

set(CMAKE_C_STANDARD 99)

add_library(oeaf STATIC
    src/oeaf_core.c
    src/oeaf_dispatch.c
    src/oeaf_sdl.c
    src/oeaf_pixels.c
    src/oeaf_rwops.c
    src/oeaf_image.c
    src/oeaf_ttf.c
    src/oeaf_mixer.c
    src/oeaf_sdl_audio.c
    backends/null/oeaf_null_backend.c
)

target_include_directories(oeaf PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# OEAF's own compilation units include backend headers by relative path.
target_include_directories(oeaf PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(oeaf_demo examples/oeaf_demo/demo.c)
target_link_libraries(oeaf_demo oeaf)
target_include_directories(oeaf_demo PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
