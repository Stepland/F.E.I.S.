project(
    'F.E.I.S.',
    'cpp',
    meson_version : '>=0.55.0',
    version : '1.1.0',
    default_options : ['cpp_std=c++20'],
)

sources = []

# I chose to put the .cpp files of the libs I vendor directly in include/
# I store the files in a (lib name -> files) dict so that tests can
# select which libs they want to compile with
include_sources = {}

subdir('include')  # Defines `inc` and adds stuff in `include_sources`
subdir('src')  # Adds stuff in `sources`

deps = [
    dependency('sfml-system', version : '>=2.5.1'),
    dependency('sfml-window', version : '>=2.5.1'),
    dependency('sfml-graphics', version : '>=2.5.1'),
    dependency('sfml-audio', version : '>=2.5.1'),
    dependency('gl'),
    dependency('openal'),
    dependency('boost'),
    dependency('mpdec++', fallback: ['mpdecimal', 'mpdecpp_dep']),
    dependency('nowide')
]

subdir('tests')

foreach lib, lib_sources : include_sources
    sources += lib_sources
endforeach

executable(
    'FEIS',
    sources: sources,
    dependencies: deps,
    include_directories: inc,
)

