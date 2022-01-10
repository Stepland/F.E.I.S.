project(
    'soloud',
    'c',
    'cpp',
    version: '20200207',
    license: 'zlib',
    default_options : ['cpp_std=c++14'],
    meson_version : '>=0.60.3',
)

cpp_args = []
dependencies = []
sources = []

subdir('src')  # fills in all the above lists

includes = include_directories('include')

if target_machine.system() != 'windows'
    dependencies += dependency('threads')
    compiler = meson.get_compiler('cpp')
    dependencies += compiler.find_library('dl')
endif

soloud_lib = static_library(
    'soloud',
    sources,
    include_directories: includes,
    dependencies: dependencies,
    cpp_args: cpp_args,
)

soloud_dep = declare_dependency(
    include_directories: includes,
    link_with: soloud_lib,
    dependencies: dependencies,
)

meson.override_dependency('soloud', soloud_dep)