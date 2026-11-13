#include "Appointment.h"
#include "../utils/ClinicException.h"
#include "../utils/Validator.h"

#include <utility>

namespace scapms::model {

std::string statusToString(AppointmentStatus status) {
    switch (status) {
        case AppointmentStatus::Pending: return "Pending";
        case AppointmentStatus::Completed: return "Completed";
        case AppointmentStatus::Cancelled: return "Cancelled";
    }
    return "Pending";
}

AppointmentStatus statusFromString(const std::string& text) {
    const std::string t = utils::Validator::toLower(utils::Validator::trim(text));
    if (t == "pending") return AppointmentStatus::Pending;
    if (t == "completed") return AppointmentStatus::Completed;
    if (t == "cancelled" || t == "canceled") return AppointmentStatus::Cancelled;
    throw utils::ValidationException("Unknown appointment status: " + text);
}

Appointment::Appointment(std::string id, std::string patientId, std::string date, std::string time,
                         std::string reason, AppointmentStatus status)
    : id_(std::move(id)), patientId_(std::move(patientId)), date_(std::move(date)), time_(std::move(time)),
      reason_(std::move(reason)), status_(status) {
    utils::Validator::validateDate(date_);
    utils::Validator::validateTime(time_);
    if (utils::Validator::trim(reason_).empty())
        throw utils::ValidationException("Appointment reason cannot be empty.");
}

} // namespace scapms::model
