#include "GeneralPatient.h"
#include "../utils/Validator.h"

#include <utility>

namespace scapms::model {

GeneralPatient::GeneralPatient(std::string id, std::string name, int age, std::string contact,
                               std::string department, std::string insuranceId)
    : PatientRecord(std::move(id), std::move(name), age, std::move(contact)),
      department_(utils::Validator::trim(department)), insuranceId_(utils::Validator::trim(insuranceId)) {
    if (department_.empty()) department_ = "General Medicine";
}

std::string GeneralPatient::getSummary() const {
    std::string s = getName() + " (" + std::to_string(getAge()) + ") - routine care in " + department_;
    s += insuranceId_.empty() ? ", self-paying" : ", insurance " + insuranceId_;
    return s;
}

std::vector<std::string> GeneralPatient::getExtraFieldLabels() const { return {"Department", "Insurance ID"}; }

std::vector<std::string> GeneralPatient::getExtraFields() const { return {department_, insuranceId_}; }

std::unique_ptr<PatientRecord> GeneralPatient::clone() const { return std::make_unique<GeneralPatient>(*this); }

void GeneralPatient::scheduleAppointment(const Appointment& appointment) {
    addAppointmentChecked(appointment, maxPendingAppointments());
}

bool GeneralPatient::cancelAppointment(const std::string& appointmentId) { return markCancelled(appointmentId); }

} // namespace scapms::model
