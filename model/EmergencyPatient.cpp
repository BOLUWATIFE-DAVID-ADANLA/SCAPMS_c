#include "EmergencyPatient.h"
#include "../utils/Validator.h"

#include <utility>

namespace scapms::model {

EmergencyPatient::EmergencyPatient(std::string id, std::string name, int age, std::string contact, int severity,
                                   std::string condition)
    : PatientRecord(std::move(id), std::move(name), age, std::move(contact)), severity_(severity),
      condition_(utils::Validator::trim(condition)) {
    utils::Validator::validateSeverity(severity_);
    if (condition_.empty()) condition_ = "Unspecified";
}

std::string EmergencyPatient::getSummary() const {
    return getName() + " (" + std::to_string(getAge()) + ") - EMERGENCY: " + condition_ + ", severity " +
           std::to_string(severity_) + "/5" + (isCritical() ? " [CRITICAL]" : "");
}

std::vector<std::string> EmergencyPatient::getExtraFieldLabels() const { return {"Severity (1-5)", "Condition"}; }

std::vector<std::string> EmergencyPatient::getExtraFields() const {
    return {std::to_string(severity_), condition_};
}

std::unique_ptr<PatientRecord> EmergencyPatient::clone() const { return std::make_unique<EmergencyPatient>(*this); }

void EmergencyPatient::scheduleAppointment(const Appointment& appointment) {
    addAppointmentChecked(appointment, maxPendingAppointments());
}

bool EmergencyPatient::cancelAppointment(const std::string& appointmentId) { return markCancelled(appointmentId); }

} // namespace scapms::model
