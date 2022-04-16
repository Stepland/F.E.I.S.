doctest_tests = executable(
    'doctest_tests',
    'main.cpp',
    'fractions.cpp',
    'decimals.cpp',
    'notes.cpp',
    'long_note_dummy.cpp',
    '../../better_beats.cpp',
    '../../better_note.cpp',
    '../../better_notes.cpp',
    '../../better_timing.cpp',
    '../../long_note_dummy.cpp',
    '../../json_decimal_handling.cpp',
    '../../special_numeric_types.cpp',
    include_sources['fmt'],
    dependencies: [
        gmp_dep,
        mpdecpp_dep,
        sfml_system_dep,  # for sf::Time mostly
    ],
    include_directories: inc,
)

test(
    'doctest',
    doctest_tests,
    args: ['--force-colors=true'],
    verbose: true,
)