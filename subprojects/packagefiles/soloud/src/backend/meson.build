# dep: the backend requires this underlying lib
# static: the backend's underlying lib is static
# dll: the backend's sources have an extra "soloud_{backend}_dll.c" file
# name: the name portion of the .cpp file is different from the dict key
backends = {
    'alsa': {'dep': 'asound'},
    'coreaudio': {'dep': 'AudioToolbox.framework'},
    'jack': {'dep': 'jack'},
    'miniaudio': {},
    'nosound': {},
    'null': {},
    'openal': {'dep': 'openal', 'dll': true},
    'oss': {},
    'portaudio': {'dep': 'portaudiocpp', 'dll': true},
    'sdl_static': {'dep': 'sdl', 'static': true},
    'sdl': {'dep': 'sdl', 'dll': true, 'name': 'sdl1'},
    'sdl2_static': {'dep': 'sdl2', 'static': true},
    'sdl2': {'dep': 'sdl2', 'dll': true},
    'vita_homebrew': {},
    'wasapi': {},
    'winmm': {},
    'xaudio2': {'dep': 'dxsdk'},
}

foreach backend_name, specifics : backends
    backend_feature = get_option(backend_name)
    summary(
        backend_name,
        backend_feature,
        section: 'Backends',
    )
    if backend_feature.allowed()
        # Find underlying lib if needed
        if specifics.has_key('dep')
            dep = dependency(
                specifics.get('dep'),
                required: backend_feature,
                static: specifics.get('static', false),
            )
            if dep.found()
                dependencies += dep
            else
                # If we are here it means the feature option of this specific
                # backend was set to auto and we did not find the underlying
                # lib anywhere. I choose to interpret 'auto' as : "If you find
                # it that's cool, if not that's also cool" so just ignore the
                # rest and don't add the files
                continue
            endif
        endif

        # Add sources
        filename = specifics.get('name', backend_name)
        cpp_file = 'soloud_' + filename + '.cpp'
        sources += files(backend_name / cpp_file)

        if specifics.get('dll', false)
            dll_c_file = 'soloud_' + filename + '_dll.c'
            sources += files(backend_name / dll_c_file)
        endif

        # Add macro
        backend_defines += '-DWITH_' + backend_name.to_upper()
    endif
endforeach

summary(
    'cpp_args',
    backend_defines,
    section: 'Backends defines',
)
