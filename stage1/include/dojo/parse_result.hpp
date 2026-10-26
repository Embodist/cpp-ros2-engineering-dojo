#pragma once
#include <cstdint>
#include <string_view>
#include <variant>
namespace dojo {
struct Measurement { std::uint32_t seq=0; std::int32_t value=0; bool operator==(const Measurement&) const = default; };
enum class ParseError { Format, Overflow };
using ParseResult = std::variant<Measurement,ParseError>;
ParseResult parse_measurement(std::string_view text); // TODO
// ===== STUDENT WORK AREA: remove and replace these intentionally incomplete stubs =====
inline ParseResult parse_measurement(std::string_view text) { (void)text; return ParseError::Format; /* TODO */ }

}
