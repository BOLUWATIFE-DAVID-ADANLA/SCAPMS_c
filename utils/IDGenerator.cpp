#include "IDGenerator.h"

#include <cctype>
#include <utility>

namespace scapms::utils {

IDGenerator::IDGenerator(std::string prefix, int width) : prefix_(std::move(prefix)), width_(width) {}

std::string IDGenerator::format(int value) const {
    std::string digits = std::to_string(value);
    while (static_cast<int>(digits.size()) < width_) digits.insert(digits.begin(), '0');
    return prefix_ + digits;
}

std::string IDGenerator::next() { return format(++counter_); }

std::string IDGenerator::peek() const { return format(counter_ + 1); }

void IDGenerator::observe(const std::string& existingId) {
    if (existingId.size() <= prefix_.size() || existingId.compare(0, prefix_.size(), prefix_) != 0) return;
    const std::string rest = existingId.substr(prefix_.size());
    if (rest.size() > 9) return;
    for (char c : rest) {
        if (!std::isdigit(static_cast<unsigned char>(c))) return;
    }
    const int value = std::stoi(rest);
    if (value > counter_) counter_ = value;
}

void IDGenerator::reset() { counter_ = 0; }

} // namespace scapms::utils
