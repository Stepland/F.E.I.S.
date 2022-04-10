rapidcheck_tests = executable(
    'rapidcheck_tests',
    'main.cpp',
    '../../better_note.cpp',
    '../../better_beats.cpp',
    '../../special_numeric_types.cpp',
    include_sources['fmt'],
    dependencies: [
        gmp_dep,
        mpdecpp_dep,
        dependency('rapidcheck')
    ],
    include_directories: inc,
)

test(
    'rapidcheck',
    rapidcheck_tests,
    verbose: true,
)