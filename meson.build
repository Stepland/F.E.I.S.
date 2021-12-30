project(
    'F.E.I.S.',
    'cpp',
    meson_version : '>=0.55.0',
    version : '1.1.0',
    default_options : ['cpp_std=c++17'],
)

sources = []

subdir('include')  # Defines `inc` and adds stuff in `sources`
subdir('src')  # Adds stuff in `sources`

deps = [
    dependency('sfml-system', version : '>=2.5.1'),
    dependency('sfml-window', version : '>=2.5.1'),
    dependency('sfml-graphics', version : '>=2.5.1'),
    dependency('sfml-audio', version : '>=2.5.1'),
    dependency('gl'),
]

executable(
    'FEIS',
    sources: sources,
    dependencies: deps,
    include_directories: inc,
)

