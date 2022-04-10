project(
    'rapidcheck',
    'cpp',
    version: '0.1.0',
    license: 'BSD-2-Clause',
    default_options : [
        'cpp_std=c++11',
        'warning_level=3',
    ],
    meson_version : '>=0.62.0',
)

sources = []

subdir('src')  # fills in 'sources'

includes = include_directories('include')

add_project_arguments(
    '-Wno-missing-braces',
    '-Wno-unused-command-line-argument',
    language: 'cpp'
)

# Random is used a LOT so it should preferably be really fast
rapidcheck_random_lib = static_library(
    'rapidcheck_random',
    'src/Random.cpp',
    include_directories: includes,
    cpp_args: '-O3',
)

# On Windows under MinGW, random_device provides no entropy,
# so it will always return the same value.
# Seed using system time instead.
# See: https://stackoverflow.com/questions/18880654/why-do-i-get-the-same-sequence-for-every-run-with-stdrandom-device-with-mingw
if host_machine.system() == 'cygwin'
    add_project_arguments('-DRC_SEED_SYSTEM_TIME', language: 'cpp')
endif

if not get_option('rtti')
    add_project_arguments('-DRC_DONT_USE_RTTI', language: 'cpp')
endif

# if get_option('tests')
#     subdir('ext')
#     subdir('test')
# endif

# if get_option('examples')
#     ... more unfun stuff
# endif

rapidcheck_lib = static_library(
    'rapidcheck',
    sources,
    include_directories: includes,
    link_with: rapidcheck_random_lib,
    cpp_args: [
        '-Wall',
        '-Wno-missing-braces',
        '-Wno-unused-command-line-argument',
    ]
)

rapidcheck_dep = declare_dependency(
    include_directories: includes,
    link_with: rapidcheck_lib,
)

meson.override_dependency('rapidcheck', rapidcheck_dep)