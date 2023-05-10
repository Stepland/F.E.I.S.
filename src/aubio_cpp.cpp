#include "aubio_cpp.hpp"

#include <algorithm>
#include <iterator>
#include <limits>

#include <aubio/fvec.h>

namespace aubio {
    std::optional<std::size_t> onset_detector::detect(const std::vector<sf::Int16>& samples) {
        std::unique_ptr<fvec_t, decltype(&del_fvec)> out{new_fvec(2), del_fvec};
        std::vector<float> input;
        std::ranges::transform(samples, std::back_inserter(input), [](const sf::Int16 i) -> float {
            return static_cast<float>(i) / std::numeric_limits<sf::Int16>::max();
        });
        fvec_t _input = {
            .length=static_cast<uint_t>(input.size()),
            .data=input.data(),
        };
        aubio_onset_do(this->get(), &_input, out.get());
        if (out->data[0] != 0) {
            return aubio_onset_get_last(this->get());
        } else {
            return {};
        }
    }
}