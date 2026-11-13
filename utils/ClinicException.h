#pragma once

#include <stdexcept>
#include <string>
#include <utility>

namespace scapms::utils {

// Base exception for every recoverable error in SCAPMS.
// Carries a short title that the GUI uses for dialog captions.
class ClinicException : public std::runtime_error {
public:
    explicit ClinicException(const std::string& message, std::string title = "Clinic Error")
        : std::runtime_error(message), title_(std::move(title)) {}

    const std::string& title() const noexcept { return title_; }

private:
    std::string title_;
};

// Invalid user input: bad age, empty name, malformed date, bad search keyword...
class ValidationException : public ClinicException {
public:
    explicit ValidationException(const std::string& message)
        : ClinicException(message, "Invalid Input") {}
};

// Attempt to register a patient whose ID is already in use.
class DuplicateIdException : public ClinicException {
public:
    explicit DuplicateIdException(const std::string& id)
        : ClinicException("Patient ID already exists: " + id, "Duplicate Patient ID"), id_(id) {}

    const std::string& id() const noexcept { return id_; }

private:
    std::string id_;
};

// A patient or appointment that was expected to exist could not be found.
class NotFoundException : public ClinicException {
public:
    explicit NotFoundException(const std::string& message)
        : ClinicException(message, "Not Found") {}
};

// Saving or loading a data file failed.
class FileException : public ClinicException {
public:
    explicit FileException(const std::string& message)
        : ClinicException(message, "File Error") {}
};

// Invalid queue or undo operation (empty queue, nothing to undo, already queued...).
class OperationException : public ClinicException {
public:
    explicit OperationException(const std::string& message)
        : ClinicException(message, "Operation Not Allowed") {}
};

} // namespace scapms::utils
