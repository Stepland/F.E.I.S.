doctest_tests = executable(
    'doctest_tests',
    'main.cpp',
    'fractions.cpp',
    'decimals.cpp',
    '../../special_numeric_types.cpp',
    include_sources['fmt'],
    dependencies: [
        gmp_dep,
        mpdecpp_dep,
    ],
    include_directories: inc,
)

test(
    'doctest',
    doctest_tests,
    args: ['--force-colors=true'],
    verbose: true,
)