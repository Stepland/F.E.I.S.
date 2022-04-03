project(
    'nowide',
    'cpp',
    version: '11.1.4',
    license: 'BSL-1.0',
    default_options : ['cpp_std=c++14'],
    meson_version : '>=0.60.3',
)

sources = []

subdir('src')  # fills in 'sources'

includes = include_directories('include')

nowide_lib = static_library(
    'nowide',
    sources,
    include_directories: includes,
)

nowide_dep = declare_dependency(
    include_directories: includes,
    link_with: nowide_lib,
)

meson.override_dependency('nowide', nowide_dep)