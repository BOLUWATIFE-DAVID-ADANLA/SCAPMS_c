#include "Validator.h"
#include "ClinicException.h"

#include <cctype>

namespace scapms::utils::Validator {

std::string trim(const std::string& text) {
    std::size_t start = 0;
    while (start < text.size() && std::isspace(static_cast<unsigned char>(text[start]))) ++start;
    std::size_t end = text.size();
    while (end > start && std::isspace(static_cast<unsigned char>(text[end - 1]))) --end;
    return text.substr(start, end - start);
}

std::string toLower(const std::string& text) {
    std::string out = text;
    for (char& c : out) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return out;
}

bool startsWithIgnoreCase(const std::string& text, const std::string& prefix) {
    if (prefix.size() > text.size()) return false;
    for (std::size_t i = 0; i < prefix.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(text[i])) !=
            std::tolower(static_cast<unsigned char>(prefix[i]))) {
            return false;
        }
    }
    return true;
}

bool containsIgnoreCase(const std::string& text, const std::string& keyword) {
    return toLower(text).find(toLower(keyword)) != std::string::npos;
}

void validatePatientId(const std::string& id) {
    if (id.empty()) throw ValidationException("Patient ID cannot be empty.");
    if (id.size() < 3 || id.size() > 12)
        throw ValidationException("Patient ID must be between 3 and 12 characters.");
    for (char c : id) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '-')
            throw ValidationException("Patient ID may contain only letters, digits and '-'.");
    }
}

void validateName(const std::string& name) {
    const std::string t = trim(name);
    if (t.empty()) throw ValidationException("Patient name cannot be empty.");
    if (t.size() < 2) throw ValidationException("Patient name must have at least 2 characters.");
    if (t.size() > 60) throw ValidationException("Patient name cannot exceed 60 characters.");
    for (char c : t) {
        const auto uc = static_cast<unsigned char>(c);
        if (!std::isalpha(uc) && c != ' ' && c != '-' && c != '\'' && c != '.')
            throw ValidationException("Patient name may contain only letters, spaces, '-', '.' and apostrophes.");
    }
}

void validateAge(int age) {
    if (age < 0 || age > 120) throw ValidationException("Patient age must be between 0 and 120.");
}

int parseAge(const std::string& text) {
    const std::string t = trim(text);
    if (t.empty()) throw ValidationException("Patient age is required.");
    for (char c : t) {
        if (!std::isdigit(static_cast<unsigned char>(c)))
            throw ValidationException("Patient age must be a whole number between 0 and 120.");
    }
    if (t.size() > 3) throw ValidationException("Patient age must be between 0 and 120.");
    const int age = std::stoi(t);
    validateAge(age);
    return age;
}

void validateContact(const std::string& contact) {
    const std::string t = trim(contact);
    if (t.empty()) throw ValidationException("Contact information is required (phone number or email).");

    const auto at = t.find('@');
    if (at != std::string::npos) {
        const auto dot = t.find('.', at);
        if (at == 0 || dot == std::string::npos || dot == at + 1 || dot == t.size() - 1)
            throw ValidationException("Email address is not valid: " + t);
        return;
    }

    int digits = 0;
    for (std::size_t i = 0; i < t.size(); ++i) {
        const char c = t[i];
        if (std::isdigit(static_cast<unsigned char>(c))) {
            ++digits;
        } else if (!(c == '+' && i == 0) && c != ' ' && c != '-') {
            throw ValidationException("Phone number may contain only digits, spaces, '-' and a leading '+'.");
        }
    }
    if (digits < 7 || digits > 15)
        throw ValidationException("Phone number must contain between 7 and 15 digits.");
}

bool isLeapYear(int year) {
    return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

void validateDate(const std::string& date) {
    if (date.size() != 10 || date[4] != '-' || date[7] != '-')
        throw ValidationException("Date must be in the format YYYY-MM-DD.");
    for (int i : {0, 1, 2, 3, 5, 6, 8, 9}) {
        if (!std::isdigit(static_cast<unsigned char>(date[i])))
            throw ValidationException("Date must be in the format YYYY-MM-DD.");
    }
    const int year = std::stoi(date.substr(0, 4));
    const int month = std::stoi(date.substr(5, 2));
    const int day = std::stoi(date.substr(8, 2));
    if (year < 1900 || year > 2100) throw ValidationException("Year must be between 1900 and 2100.");
    if (month < 1 || month > 12) throw ValidationException("Month must be between 01 and 12.");
    static const int daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int maxDay = daysInMonth[month - 1];
    if (month == 2 && isLeapYear(year)) maxDay = 29;
    if (day < 1 || day > maxDay) throw ValidationException("Invalid day for the given month: " + date);
}

void validateTime(const std::string& time) {
    if (time.size() != 5 || time[2] != ':' ||
        !std::isdigit(static_cast<unsigned char>(time[0])) || !std::isdigit(static_cast<unsigned char>(time[1])) ||
        !std::isdigit(static_cast<unsigned char>(time[3])) || !std::isdigit(static_cast<unsigned char>(time[4])))
        throw ValidationException("Time must be in the format HH:MM (24-hour).");
    const int h = std::stoi(time.substr(0, 2));
    const int m = std::stoi(time.substr(3, 2));
    if (h > 23 || m > 59) throw ValidationException("Time is out of range: " + time);
}

void validateSeverity(int severity) {
    if (severity < 1 || severity > 5) throw ValidationException("Emergency severity must be between 1 and 5.");
}

int parseSeverity(const std::string& text) {
    const std::string t = trim(text);
    if (t.size() != 1 || !std::isdigit(static_cast<unsigned char>(t[0])))
        throw ValidationException("Emergency severity must be a number between 1 and 5.");
    const int s = t[0] - '0';
    validateSeverity(s);
    return s;
}

} // namespace scapms::utils::Validator
