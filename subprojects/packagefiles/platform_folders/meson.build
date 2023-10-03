project(
    'platform_folders',
    'cpp',
    version: '4.2.0',
    license: 'MIT',
    default_options : ['cpp_std=c++11'],
    meson_version : '>=0.60.3',
)

sources = files('sago/platform_folders.cpp')

includes = include_directories('.')

platform_folders_lib = library(
    'platform_folders',
    sources,
    include_directories: includes,
)

platform_folders_dep = declare_dependency(
    include_directories: includes,
    link_with: platform_folders_lib,
)

meson.override_dependency('platform_folders', platform_folders_dep)