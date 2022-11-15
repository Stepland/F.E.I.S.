sources += files(
    'better_beats.cpp',
    'better_chart.cpp',
    'better_hakus.cpp',
    'better_metadata.cpp',
    'better_note.cpp',
    'better_notes.cpp',
    'better_song.cpp',
    'better_timing.cpp',
    'chart.cpp',
    'chart_state.cpp',
    'clipboard.cpp',
    'editor_state.cpp',
    'file_dialogs.cpp',
    'fumen.cpp',
    'history_item.cpp',
    'imgui_extras.cpp',
    'json_decimal_handling.cpp',
    'ln_marker.cpp',
    'long_note_dummy.cpp',
    'main.cpp',
    'marker.cpp',
    'mp3_reader.cpp',
    'note.cpp',
    'notification.cpp',
    'notifications_queue.cpp',
    'playfield.cpp',
    'preferences.cpp',
    'sound_effect.cpp',
    'special_numeric_types.cpp',
    'toolbox.cpp',
)

conf_data = configuration_data()

conf_data.set('name', meson.project_name())
conf_data.set('version', meson.project_version())

configure_file(
    input: 'compile_time_info.hpp.in',
    output: 'compile_time_info.hpp',
    configuration: conf_data,
)

configuration_inc = include_directories('.')


subdir('custom_sfml_audio')
subdir('widgets')

if get_option('tests')
    subdir('tests')
endif