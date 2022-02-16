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