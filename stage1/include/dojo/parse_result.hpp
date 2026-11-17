#pragma once
#include <charconv>
#include <cstdint>
#include <string_view>
#include <system_error>
#include <variant>
namespace dojo {
struct Measurement { std::uint32_t seq=0; std::int32_t value=0; bool operator==(const Measurement&) const = default; };
enum class ParseError { Format, Overflow };
using ParseResult = std::variant<Measurement,ParseError>;
ParseResult parse_measurement(std::string_view text); // TODO
inline ParseResult parse_measurement(std::string_view text) {
    constexpr std::string_view prefix = "seq:";
    constexpr std::string_view separator = ";value:";
    if (!text.starts_with(prefix)) return ParseError::Format;

    const std::size_t sep = text.find(separator, prefix.size());
    if (sep == std::string_view::npos) return ParseError::Format;

    const std::string_view seq_text = text.substr(prefix.size(), sep - prefix.size());
    const std::string_view value_text = text.substr(sep + separator.size());
    if (seq_text.empty() || value_text.empty()) return ParseError::Format;

    std::uint32_t seq = 0;
    const auto seq_result =
        std::from_chars(seq_text.data(), seq_text.data() + seq_text.size(), seq);
    if (seq_result.ec != std::errc{}) {
        return seq_result.ec == std::errc::result_out_of_range
                   ? ParseResult{ParseError::Overflow}
                   : ParseResult{ParseError::Format};
    }
    if (seq_result.ptr != seq_text.data() + seq_text.size()) {
        return ParseError::Format;
    }

    std::int32_t value = 0;
    const auto value_result =
        std::from_chars(value_text.data(), value_text.data() + value_text.size(), value);
    if (value_result.ec != std::errc{}) {
        return value_result.ec == std::errc::result_out_of_range
                   ? ParseResult{ParseError::Overflow}
                   : ParseResult{ParseError::Format};
    }
    if (value_result.ptr != value_text.data() + value_text.size()) {
        return ParseError::Format;
    }

    return Measurement{seq, value};
}

}
