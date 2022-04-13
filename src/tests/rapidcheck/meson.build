rapidcheck_tests = executable(
    'rapidcheck_tests',
    'generators.cpp',
    'main.cpp',
    '../../better_beats.cpp',
    '../../better_note.cpp',
    '../../better_notes.cpp',
    '../../better_timing.cpp',
    '../../special_numeric_types.cpp',
    include_sources['fmt'],
    dependencies: [
        gmp_dep,
        mpdecpp_dep,
        sfml_system_dep,
        dependency('rapidcheck')
    ],
    include_directories: inc,
)

test(
    'rapidcheck',
    rapidcheck_tests,
    verbose: true,
)