#pragma once

#include <compare>
#include <cstdint>

namespace pumpdx::chart {

class Beat final {
public:
    Beat(std::int64_t numerator = 0, std::int64_t denominator = 1);

    [[nodiscard]] static Beat Zero() noexcept;
    [[nodiscard]] std::int64_t Numerator() const noexcept;
    [[nodiscard]] std::int64_t Denominator() const noexcept;
    [[nodiscard]] double ToDouble() const noexcept;
    [[nodiscard]] Beat operator+(const Beat& other) const;
    [[nodiscard]] Beat operator-(const Beat& other) const;

    [[nodiscard]] std::strong_ordering operator<=>(const Beat& other) const noexcept;
    [[nodiscard]] bool operator==(const Beat& other) const noexcept;

private:
    std::int64_t numerator_ = 0;
    std::int64_t denominator_ = 1;
};

} // namespace pumpdx::chart
