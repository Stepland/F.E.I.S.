executable(
    'imgui_demo',
    'imgui_demo.cpp',
    include_sources['imgui'],
    include_sources['imgui-demo'],
    dependencies: deps,
    include_directories: inc,
)

executable(
    'imgui_input_decimal',
    'imgui_input_decimal.cpp',
    include_sources['imgui'],
    dependencies: deps,
    include_directories: inc,
)

executable(
    'json_decimal',
    'json_decimal.cpp',
    dependencies: deps,
    include_directories: inc,
)

executable(
    'scrollwheel',
    'scrollwheel.cpp',
    include_sources['imgui'],
    dependencies: deps,
    include_directories: inc,
)

executable(
    'mouse_events',
    'mouse_events.cpp',
    dependencies: deps,
    include_directories: inc,
)

executable(
    'more_precise_music',
    'more_precise_music.cpp',
    dependencies: deps + [dependency('openal')],
    include_directories: inc,
)