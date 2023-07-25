#include "better_timing.hpp"

#include <algorithm>
#include <bits/ranges_algo.h>
#include <iterator>
#include <memory>
#include <string>

#include <fmt/core.h>
#include <json.hpp>

#include "better_beats.hpp"
#include "imgui.h"
#include "json_decimal_handling.hpp"
#include "toolbox.hpp"

namespace better {
    BPMAtBeat::BPMAtBeat(Decimal bpm_, Fraction beats_) :
        bpm(bpm_),
        bpm_as_double(std::stod(bpm_.format("f"))),
        beats(beats_)
    {
        if (bpm <= 0) {
            std::stringstream ss;
            ss << "Attempted to create a BPMAtBeat with negative BPM : ";
            ss << bpm;
            throw std::invalid_argument(ss.str());
        }
    };

    Decimal BPMAtBeat::get_bpm() const {
        return bpm;
    }

    double BPMAtBeat::get_bpm_as_double() const {
        return bpm_as_double;
    }
    
    Fraction BPMAtBeat::get_beats() const {
        return beats;
    }

    BPMEvent::BPMEvent(Fraction beats_, double seconds_, Decimal bpm_) :
        bpm(bpm_),
        bpm_as_double(std::stod(bpm_.format("f"))),
        beats(beats_),
        seconds(seconds_)
    {};

    Decimal BPMEvent::get_bpm() const {
        return bpm;
    }

    double BPMEvent::get_bpm_as_double() const {
        return bpm_as_double;
    }
    
    Fraction BPMEvent::get_beats() const {
        return beats;
    }

    double BPMEvent::get_seconds() const {
        return seconds;
    };

    std::ostream& operator<<(std::ostream& out, const BPMEvent& b) {
        out << fmt::format("{}", b);
        return out;
    }
    

    // Default constructor, used when creating a new song from scratch
    Timing::Timing() :
        Timing({{120, 0}}, 0)
    {};
    
    /*
    Create a Time Map from a list of BPM changes with times given in
    beats, and the beat zero offset
    */
    Timing::Timing(const std::vector<BPMAtBeat>& events, const Decimal& offset_) {
        reconstruct(events, offset_);
    }

    void Timing::display_as_imgui_table(const std::string& name) {
        if(
            ImGui::BeginTable(
                name.c_str(),
                2,
                ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit
            )
        ) {
            ImGui::TableSetupColumn("Key");
            ImGui::TableSetupColumn("Value");
            ImGui::TableHeadersRow();
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("offset");
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(offset.format(".3f").c_str());
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("bpm");
            ImGui::TableNextColumn();
            for (const auto& bpm : events_by_beats) {
                ImGui::TextUnformatted(fmt::format("{} @ {:.3f}", bpm.get_bpm().format(".3f"), static_cast<double>(bpm.get_beats())).c_str());
            }
            ImGui::EndTable();
        }
    }

    /*
    Return the amount of seconds at the given beat.

    Before the first bpm change, compute backwards from the first bpm,
    after the first bpm change, compute forwards from the previous bpm
    change
    */
    double Timing::seconds_at(Fraction beats) const {
        return offset_as_double + seconds_without_offset_at(beats);
    };

    double Timing::seconds_without_offset_at(Fraction beats) const {
        const auto& bpm_change = bpm_event_in_effect_at(beats);
        const Fraction beats_since_previous_event = beats - bpm_change.get_beats();
        double seconds_since_previous_event = static_cast<double>(beats_since_previous_event) * 60 / bpm_change.get_bpm_as_double();
        const auto previous_event_seconds = bpm_change.get_seconds();
        return previous_event_seconds + seconds_since_previous_event;
    }

    double Timing::seconds_between(Fraction beat_a, Fraction beat_b) const {
        return seconds_at(beat_b) - seconds_at(beat_a);
    };

    sf::Time Timing::time_at(Fraction beats) const {
        return sf::seconds(seconds_at(beats));
    };

    sf::Time Timing::time_between(Fraction beat_a, Fraction beat_b) const {
        return sf::seconds(seconds_between(beat_a, beat_b));
    };

    Fraction Timing::beats_at(sf::Time time) const {
        const auto seconds = static_cast<double>(time.asMicroseconds()) / 1000000.0;
        return beats_at(seconds);
    };

    Fraction Timing::beats_at(double seconds) const {
        const auto& bpm_change = bpm_event_in_effect_at(seconds);
        const auto previous_event_seconds = bpm_change.get_seconds() + offset_as_double;
        const auto seconds_since_previous_event = seconds - previous_event_seconds;
        const auto beats_since_previous_event = (
            convert_to_fraction(bpm_change.get_bpm())
            * Fraction{seconds_since_previous_event}
            / 60
        );
        return bpm_change.get_beats() + beats_since_previous_event;
    };

    Decimal Timing::bpm_at(sf::Time time) const {
        const auto seconds = static_cast<double>(time.asMicroseconds()) / 1000000.0;
        return bpm_at(seconds);
    }

    Decimal Timing::bpm_at(double seconds) const {
        const auto& bpm_change = bpm_event_in_effect_at(seconds);
        return bpm_change.get_bpm();
    }

    Decimal Timing::bpm_at(Fraction beats) const {
        const auto& bpm_change = bpm_event_in_effect_at(beats);
        return bpm_change.get_bpm();
    }

    void Timing::insert(const BPMAtBeat& new_bpm) {
        std::vector<BPMAtBeat> new_events;
        new_events.reserve(events_by_beats.size() + 1);
        // put the new bpm in front so it's kept instead of existing events at the same beat
        new_events.push_back(new_bpm);
        std::transform(
            events_by_beats.begin(),
            events_by_beats.end(),
            std::back_inserter(new_events),
            [&](const key_type& bpm){
                return BPMAtBeat{bpm.get_bpm(), bpm.get_beats()};
            }
        );
        reconstruct(new_events, offset);
    }

    void Timing::erase(const BPMAtBeat& event) {
        if (events_by_beats.size() == 1) {
            return;
        }
        events_by_beats.erase({event.get_beats(), 0, 0});
        std::vector<BPMAtBeat> new_events;
        new_events.reserve(events_by_beats.size());
        std::transform(
            events_by_beats.begin(),
            events_by_beats.end(),
            std::back_inserter(new_events),
            [&](const key_type& bpm){
                return BPMAtBeat{bpm.get_bpm(), bpm.get_beats()};
            }
        );
        reconstruct(new_events, offset);
    }


    Decimal Timing::get_offset() const {
        return offset;
    }

    void Timing::set_offset(const Decimal& new_offset) {
        offset = new_offset;
        offset_as_double = std::stod(new_offset.format("f"));
    }


    nlohmann::ordered_json Timing::dump_to_memon_1_0_0() const {
        nlohmann::ordered_json j;
        j["offset"] = offset.format("f");
        auto bpms = nlohmann::ordered_json::array();
        for (const auto& bpm_change : events_by_beats) {
            bpms.push_back({
                {"beat", beat_to_best_form(bpm_change.get_beats())},
                {"bpm", bpm_change.get_bpm().format("f")}
            });
        }
        j["bpms"] = bpms;
        return j;
    };

    Timing Timing::load_from_memon_1_0_0(const nlohmann::json& json) {
        Decimal offset = 0;
        if (json.contains("offset")) {
            offset = load_as_decimal(json["offset"]);
        }
        std::uint64_t resolution = 240;
        if (json.contains("resolution")) {
            resolution = json["resolution"].get<std::uint64_t>();
        }
        std::vector<BPMAtBeat> bpms;
        if (not json.contains("bpms")) {
            bpms = {{120, 0}};
        } else {
            for (const auto& bpm_json : json["bpms"]) {
                try {
                    bpms.emplace_back(
                        load_as_decimal(bpm_json["bpm"]),
                        load_memon_1_0_0_beat(bpm_json["beat"], resolution)
                    );
                } catch (const std::exception&) {
                    continue;
                }
            }
        }
        return {
            bpms,
            offset
        };
    };

    /*
    In legacy memon, offset is the OPPOSITE of the time (in seconds) at which
    the first beat occurs in the music file
    */
    Timing Timing::load_from_memon_legacy(const nlohmann::json& metadata) {
        const auto bpm = load_as_decimal(metadata["BPM"]);
        const auto offset = load_as_decimal(metadata["offset"]);
        return Timing{{{bpm, 0}}, -1 * offset};
    };

    Timing::keys_by_beats_type::const_iterator Timing::cbegin() const {
        return events_by_beats.cbegin();
    }

    Timing::keys_by_beats_type::const_iterator Timing::cend() const {
        return events_by_beats.cend();
    }

    void Timing::reconstruct(const std::vector<BPMAtBeat>& events, const Decimal& offset) {
        reload_events_from(events);
        set_offset(offset);
    }

    void Timing::reload_events_from(const std::vector<BPMAtBeat>& events) {
        if (events.empty()) {
            throw std::invalid_argument(
                "Attempted to create a Timing object with no BPM events"
            );
        }

        // Sort events by beat, keeping only the first in insertion order in
        // case there are multiple on a given beat
        std::set<BPMAtBeat, Timing::beat_order_for_events> sorted_events;
        for (const auto& event: events) {
            if (not sorted_events.contains(event)) {
                sorted_events.insert(event);
            }
        }

        // Only keep non-redundant bpm changes
        std::set<BPMAtBeat, Timing::beat_order_for_events> filtered_events;
        for (const auto& event : sorted_events) {
            if (filtered_events.empty()) {
                filtered_events.insert(event);
            } else {
                const auto& previous_event = filtered_events.crbegin();
                if (event.get_bpm() != previous_event->get_bpm()) {
                    filtered_events.insert(event);
                }
            }
        }

        // Compute everything as if the first BPM change happened at zero
        // seconds
        auto first_event = filtered_events.begin();
        double current_second = 0;
        events_by_beats.clear();
        events_by_beats.emplace(
            first_event->get_beats(),
            current_second,
            first_event->get_bpm()
        );

        auto previous = first_event;
        auto current = std::next(first_event);
        for (; current != filtered_events.end(); ++previous, ++current) {
            const Fraction beats_since_last_event = current->get_beats() - previous->get_beats();
            double seconds_since_last_event = static_cast<double>(beats_since_last_event) * 60 / previous->get_bpm_as_double();
            current_second += seconds_since_last_event;
            events_by_beats.emplace(
                current->get_beats(),
                current_second,
                current->get_bpm()
            );
        }

        // Shift events so their precomputed "seconds" put beat zero
        // at zero seconds
        const auto shift = seconds_without_offset_at(0);
        Timing::keys_by_beats_type shifted_events;
        seconds_to_beats.clear();
        std::for_each(
            events_by_beats.cbegin(),
            events_by_beats.cend(),
            [&](const auto& event) {
                const auto seconds = event.get_seconds() - shift;
                const auto beats = event.get_beats();
                shifted_events.emplace(beats, seconds, event.get_bpm());
                seconds_to_beats.emplace(seconds, beats);
            }
        );
        std::swap(shifted_events, events_by_beats);
    }

    const Timing::key_type& Timing::bpm_event_in_effect_at(sf::Time time) const {
        const auto seconds = static_cast<double>(time.asMicroseconds()) / 1000000.0;
        return bpm_event_in_effect_at(seconds);
    }

    const Timing::key_type& Timing::bpm_event_in_effect_at(double seconds) const {
        auto it = seconds_to_beats.upper_bound(seconds - offset_as_double);
        if (it != seconds_to_beats.begin()) {
            it = std::prev(it);
        }
        const auto [_, beat] = *it;
        return bpm_event_in_effect_at(beat);
    }

    const Timing::key_type& Timing::bpm_event_in_effect_at(Fraction beats) const {
        auto it = events_by_beats.upper_bound(bpm_event_type{beats, 0, 0});
        if (it != events_by_beats.begin()) {
            it = std::prev(it);
        }
        return *it;
    }

    std::ostream& operator<<(std::ostream& out, const Timing& t) {
        out << fmt::format("{}", t);
        return out;
    };
}