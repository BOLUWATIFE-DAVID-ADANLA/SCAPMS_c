#pragma once

#include <string>

namespace scapms::utils {

// Generates sequential, zero-padded IDs such as P0001 or A0042.
// observe() keeps the counter ahead of IDs that were loaded from file
// or typed manually, so generated IDs never collide with existing ones.
class IDGenerator {
public:
    explicit IDGenerator(std::string prefix, int width = 4);

    std::string next();
    std::string peek() const;
    void observe(const std::string& existingId);
    void reset();
    const std::string& prefix() const { return prefix_; }

private:
    std::string format(int value) const;

    std::string prefix_;
    int width_;
    int counter_ = 0;
};

} // namespace scapms::utils
