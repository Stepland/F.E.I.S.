project(
    'F.E.I.S',
    'cpp', 'c',
    meson_version : '>=0.62.0',
    version : '2.0.0-alpha',
    default_options : ['cpp_std=c++20'],
)

deps = []

sfml_system_dep = dependency('sfml-system', version : '>=2.5.1')
deps += [sfml_system_dep]
sfml_window_dep = dependency('sfml-window', version : '>=2.5.1')
deps += [sfml_window_dep]
sfml_graphics_dep = dependency('sfml-graphics', version : '>=2.5.1')
deps += [sfml_graphics_dep]
sfml_audio_dep = dependency('sfml-audio', version : '>=2.5.1')
deps += [sfml_audio_dep]
gl_dep = dependency('gl')
deps += [gl_dep]
openal_dep = dependency('openal')
deps += [openal_dep]
mpdecpp_dep = dependency('mpdec++', fallback: ['mpdecimal', 'mpdecpp_dep'])
deps += [mpdecpp_dep]
gmp_dep = dependency('gmp')
deps += [gmp_dep]
nowide_dep = dependency('nowide')
deps += [nowide_dep]
deps += dependency('threads')
tomlplusplus_dep = dependency('tomlplusplus')
deps += [tomlplusplus_dep]
aubio_dep = dependency('aubio')
deps += [aubio_dep]
fftw3_dep = dependency('fftw3')
deps += [fftw3_dep]

# I chose to put the .cpp files of the libs I vendor directly in include/
# I store the files in a (lib name -> files) dict so that tests can
# select which libs they want to compile with
include_sources = {}
subdir('include')  # Defines `inc` and adds stuff in `include_sources`

sources = []
subdir('src')  # Adds stuff in the `sources` list and defines configuration_inc

subdir('tests')

foreach lib, lib_sources : include_sources
    sources += lib_sources
endforeach

executable(
    'FEIS',
    sources: sources,
    dependencies: deps,
    include_directories: [
        inc,
        configuration_inc
    ]
)
