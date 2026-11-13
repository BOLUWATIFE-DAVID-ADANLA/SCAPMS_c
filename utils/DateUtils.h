#pragma once

#include <string>

// Small, framework-free date helpers used by the model and controller layers.
namespace scapms::utils::DateUtils {

std::string today();                                  // "YYYY-MM-DD"
std::string currentTime();                            // "HH:MM"
long long nowEpoch();                                 // seconds since epoch
std::string addDays(const std::string& date, int days);
std::string epochToTime(long long epoch);             // "HH:MM"

} // namespace scapms::utils::DateUtils
