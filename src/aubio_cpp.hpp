#pragma once

#include <functional>
#include <memory>
#include <optional>

#include <aubio/aubio.h>
#include <aubio/onset/onset.h>
#include <SFML/Config.hpp>

// Thanks stack overflow !
// https://stackoverflow.com/a/54121092/10768117
template <auto F>
struct Functor {
    template <typename... Args>
    auto operator()(Args&&... args) const { return std::invoke(F, std::forward<Args>(args)...); }
};

namespace aubio {
    using _aubio_onset_t_unique_ptr = std::unique_ptr<aubio_onset_t, Functor<del_aubio_onset>>;
    struct onset_detector : _aubio_onset_t_unique_ptr {
        template <typename... Args>
        onset_detector(Args&&... args) : _aubio_onset_t_unique_ptr(new_aubio_onset(std::forward<Args>(args)...)) {}
        // takes in Mono samples
        std::optional<std::size_t> detect(const std::vector<sf::Int16>& samples);
    };
}