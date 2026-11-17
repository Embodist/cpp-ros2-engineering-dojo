#include <gtest/gtest.h>

#include <string_view>
#include <variant>

#include "dojo/parse_result.hpp"

TEST(ParseResult, ValidMeasurement) {
    auto result = dojo::parse_measurement("seq:42;value:-7");
    ASSERT_TRUE(std::holds_alternative<dojo::Measurement>(result));
    EXPECT_EQ(std::get<dojo::Measurement>(result).seq, 42u);
    EXPECT_EQ(std::get<dojo::Measurement>(result).value, -7);
}

TEST(ParseResult, StrictFormatRejectsTrailingSpace) {
    auto result = dojo::parse_measurement("seq:1;value:2 ");
    ASSERT_TRUE(std::holds_alternative<dojo::ParseError>(result));
    EXPECT_EQ(std::get<dojo::ParseError>(result), dojo::ParseError::Format);
}

TEST(ParseResult, NonNumericIsFormat) {
    auto result = dojo::parse_measurement("seq:a;value:2");
    ASSERT_TRUE(std::holds_alternative<dojo::ParseError>(result));
    EXPECT_EQ(std::get<dojo::ParseError>(result), dojo::ParseError::Format);
}

TEST(ParseResult, OverflowIsReported) {
    auto result = dojo::parse_measurement("seq:4294967296;value:1");
    ASSERT_TRUE(std::holds_alternative<dojo::ParseError>(result));
    EXPECT_EQ(std::get<dojo::ParseError>(result), dojo::ParseError::Overflow);
}

TEST(ParseResult, TableDrivenStrictCases) {
    struct Case {
        std::string_view text;
        bool valid;
        dojo::ParseError error;
    };
    const Case cases[] = {
        {"seq:0;value:0", true, dojo::ParseError::Format},
        {"seq:4294967295;value:-2147483648", true, dojo::ParseError::Format},
        {"seq:42;value:", false, dojo::ParseError::Format},
        {"seq:-1;value:2", false, dojo::ParseError::Format},
        {"seq:1;value:+2", false, dojo::ParseError::Format},
        {"seq:1;value:2;x", false, dojo::ParseError::Format},
        {"seq:1;value:-2147483649", false, dojo::ParseError::Overflow},
    };

    for (const auto& test_case : cases) {
        SCOPED_TRACE(test_case.text);
        auto result = dojo::parse_measurement(test_case.text);
        if (test_case.valid) {
            EXPECT_TRUE(std::holds_alternative<dojo::Measurement>(result));
        } else {
            ASSERT_TRUE(std::holds_alternative<dojo::ParseError>(result));
            EXPECT_EQ(std::get<dojo::ParseError>(result), test_case.error);
        }
    }
}
