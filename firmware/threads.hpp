#pragma once

#include <algorithm>
#include <array>
#include <boost/rational_minimal.hpp>
#include <cstdio>
#include <numeric>
#include <string_view>
#include <string>
#include <type_traits>

namespace threads {
  using Rational = boost::rational<unsigned>;

  enum class pitch_type {
    mm,
    tpi,
  };

  struct pitch_info {
    std::string_view pitch_str;
    Rational value;
    pitch_type type;
  };

  namespace detail {
    // some stdlib functionality is either not constexpr or uses too much space
    constexpr unsigned sv_to_unsigned(std::string_view sv) {
      unsigned result = 0;
      for (auto i = sv.begin(); i != sv.end(); ++i) {
        uint8_t digit = *i - '0';
        result = result * 10 + digit;
      }
      return result;
    }

    constexpr unsigned pow10(unsigned power) {
      unsigned result = 1;
      for (; power > 0; --power) { result *= 10; }
      return result;
    }

    // Convert string containing decimal into rational number with no error
    // e.g. "1.865" -> 373/200
    constexpr Rational decimal_to_rational(std::string_view decimal) {
      auto i_dot = decimal.find('.');
      if (i_dot == decimal.npos) {
        return { sv_to_unsigned(decimal), 1 };
      } else {
        auto fraction = decimal;
        fraction.remove_prefix(i_dot + 1);
        auto denom = pow10(fraction.size());
        decimal.remove_suffix(decimal.size() - i_dot);
        auto num = sv_to_unsigned(decimal) * denom + sv_to_unsigned(fraction);
        return { num, denom };
      }
    }
  }

  inline namespace literals {
    constexpr pitch_info operator"" _mm(const char* pitch) {
      return { pitch, detail::decimal_to_rational(pitch), pitch_type::mm };
    }

    constexpr pitch_info operator"" _tpi(const char* pitch) {
      auto r = detail::decimal_to_rational(pitch);
      // TPI = 1 inch/#Threads : 1 inch=25.4mm = 127 / 5 mm
      return { pitch, {127 * r.denominator(), 5 * r.numerator()}, pitch_type::tpi };
    }
  }

}