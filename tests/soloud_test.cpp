

#include <chrono>
#include <iostream>
#include <thread>

#include <soloud.h>
#include <soloud_wavstream.h>

int main(int argc, char** argv) {
    using namespace std::chrono_literals;
    if (argc < 2) {
        std::cout << "usage : soloud_test [audio_file]" << '\n';
        return -1;
    }
    
    SoLoud::WavStream music;
    auto error_code = music.load(argv[1]);
    if (music.load(argv[1])) {
        std::cerr << "Could not load " << argv[1] << " ";
        std::cerr << "(error code : " << error_code << ")" << '\n';
        return -1;
    }

    SoLoud::Soloud soloud;
    soloud.init();

    std::cout << "Current backend: " << soloud.getBackendString() << '\n';

    soloud.play(music);

    // Wait until sounds have finished
    while (soloud.getActiveVoiceCount() > 0) {
        // Still going, sleep for a bit
        std::this_thread::sleep_for(1s);
    }

    soloud.deinit();
    return 0;
}