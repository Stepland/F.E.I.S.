executable(
    'hsluv_rainbow_maker',
    'hsluv_rainbow_maker.cpp',
    include_sources['imgui'],
    include_sources['hsluv'],
    dependencies: deps,
    include_directories: inc,
)

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
    'nowide',
    'nowide.cpp',
    include_sources['tinyfiledialogs'],
    dependencies: deps,
    include_directories: inc,
)

executable(
    'nowide+whereami',
    'nowide+whereami.cpp',
    include_sources['whereami'],
    include_sources['fmt'],
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