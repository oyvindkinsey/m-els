
#include "constants.hpp"
#include "thread_list.hpp"

struct Configuration {

    using Rational = threads::Rational;

    threads::thread thread = threads::pitch_list[threads::default_pitch_index];
    int16_t pitch_list_index = threads::default_pitch_index;

    Configuration() {
        rationals.encoder = {
            constants::encoder_resolution * constants::encoder_gearing.first,
            constants::encoder_gearing.second };
        rationals.steps_per_rev = {
            constants::stepper_full_steps * constants::stepper_micro_steps *
                constants::stepper_gearing.first,
            constants::stepper_gearing.second };
    }

    Rational calculate_ratio() const {
        return calculate_ratio_for_pitch(thread.pitch.value);
    }

    void cycle_thread(bool fwd) {
        // TODO: skip incompatible threads
        if (fwd) {
            auto i = pitch_list_index + 1;
            if (i == threads::pitch_list_size)
                i = 0;
            pitch_list_index = i;
        } else {
            auto i = pitch_list_index - 1;
            if (i < 0)
                i = threads::pitch_list_size - 1;
            pitch_list_index = i;
        }
        thread = threads::pitch_list[pitch_list_index];
    }

    void select_thread(int16_t new_pitch_index) {
        thread = threads::pitch_list[pitch_list_index = new_pitch_index];
    }

    enum thread_compatibility: uint8_t {
        thread_OK = 0,
        thread_too_large,
        thread_too_small
    };

    template <typename TimerCounter = uint16_t>
    thread_compatibility verify_thread(int16_t thread_index) const {
        auto ratio = calculate_ratio_for_pitch(threads::pitch_list[thread_index].pitch.value);
        auto n = ratio.numerator(), d = ratio.denominator();
        if (n >= d) {
            return thread_too_large;
        }
        if (((d + n - 1) / n) > (std::numeric_limits<TimerCounter>::max() / 2)) {
            return thread_too_small;
        }
        return thread_OK;
    }

private:
    struct Bundle {
        Rational encoder{};
        Rational steps_per_rev{};
    };

    Bundle rationals{};

    Rational calculate_ratio_for_pitch(const Rational& pitch) const {
        return (pitch / constants::leadscrew_pitch) * rationals.steps_per_rev / rationals.encoder;
    }
};