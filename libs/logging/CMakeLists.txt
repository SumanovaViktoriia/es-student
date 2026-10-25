add_library(logging STATIC
    src/log.c
)

target_include_directories(logging PUBLIC
    include
)

target_link_libraries(logging PRIVATE
    pico_stdlib
)