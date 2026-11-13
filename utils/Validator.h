#pragma once

#include <string>

// Input validation helpers. Every validate* function throws
// scapms::utils::ValidationException with a user-friendly message on failure.
namespace scapms::utils::Validator {

std::string trim(const std::string& text);
std::string toLower(const std::string& text);
bool startsWithIgnoreCase(const std::string& text, const std::string& prefix);
bool containsIgnoreCase(const std::string& text, const std::string& keyword);

void validatePatientId(const std::string& id);
void validateName(const std::string& name);
void validateAge(int age);
int parseAge(const std::string& text);          // parses then validates 0..120
void validateContact(const std::string& contact);
void validateDate(const std::string& date);     // YYYY-MM-DD, real calendar date
void validateTime(const std::string& time);     // HH:MM, 24-hour clock
void validateSeverity(int severity);            // 1..5
int parseSeverity(const std::string& text);
bool isLeapYear(int year);

} // namespace scapms::utils::Validator
