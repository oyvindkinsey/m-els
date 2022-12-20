
#include <stdint.h"

namespace devices
{

    template <uint8_t Period_ms = 10, uint8_t Samples = 16>

    struct rpm_counter
    {
        static constexpr uint16_t periods_per_min = 60000 / Period_ms;
        static constexpr uint8_t Sampling_period = Period_ms;

        volatile inline static uint16_t last_reading = 0;
        volatile inline static uint8_t sample_index = 0;
        volatile inline static unsigned running_sum = 0;
        volatile inline static uint16_t sum = 0;

        static inline uint16_t convert_sample(uint16_t c)
        {
            auto p = last_reading;
            uint16_t d = (c > p) ? (c - p) : (p - c);
            if (d > std::numeric_limits<uint16_t>::max() / 2)
            {
                d = std::numeric_limits<uint16_t>::max() - d;
            }
            last_reading = c;
            return d;
        }

        static bool process_sample(uint16_t current_reading)
        {
            running_sum += convert_sample(current_reading);
            auto i = sample_index;
            if ((++i) == Samples)
            {
                sum = running_sum;
                sample_index = 0;
                running_sum = 0;
                return true;
            }
            else
            {
                sample_index = i;
                return false;
            }
        }

        static uint16_t get_rpm(uint16_t encoder_resolution)
        {
            auto rpm = (sum * periods_per_min) / (encoder_resolution * Samples);
            return static_cast<uint16_t>(rpm);
        }
    };

} // namespace devices
