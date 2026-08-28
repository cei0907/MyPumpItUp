#include "game/chart/Beat.hpp"

#include <numeric>
#include <stdexcept>

namespace pumpdx::chart {

Beat::Beat(std::int64_t numerator, std::int64_t denominator) {
    if (denominator == 0) {
        throw std::invalid_argument("A beat denominator cannot be zero.");
    }

    if (denominator < 0) {
        numerator = -numerator;
        denominator = -denominator;
    }

    const auto divisor = std::gcd(numerator, denominator);
    numerator_ = numerator / divisor;
    denominator_ = denominator / divisor;
}

Beat Beat::Zero() noexcept {
    return {};
}

std::int64_t Beat::Numerator() const noexcept {
    return numerator_;
}

std::int64_t Beat::Denominator() const noexcept {
    return denominator_;
}

double Beat::ToDouble() const noexcept {
    return static_cast<double>(numerator_) / static_cast<double>(denominator_);
}

Beat Beat::operator+(const Beat& other) const {
    return {
        numerator_ * other.denominator_ + other.numerator_ * denominator_,
        denominator_ * other.denominator_,
    };
}

Beat Beat::operator-(const Beat& other) const {
    return {
        numerator_ * other.denominator_ - other.numerator_ * denominator_,
        denominator_ * other.denominator_,
    };
}

std::strong_ordering Beat::operator<=>(const Beat& other) const noexcept {
    const auto left = numerator_ * other.denominator_;
    const auto right = other.numerator_ * denominator_;
    return left <=> right;
}

bool Beat::operator==(const Beat& other) const noexcept {
    return numerator_ == other.numerator_ && denominator_ == other.denominator_;
}

} // namespace pumpdx::chart
