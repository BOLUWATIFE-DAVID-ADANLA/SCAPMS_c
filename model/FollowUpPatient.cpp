#include "FollowUpPatient.h"
#include "../utils/ClinicException.h"
#include "../utils/Validator.h"

#include <utility>

namespace scapms::model {

FollowUpPatient::FollowUpPatient(std::string id, std::string name, int age, std::string contact,
                                 std::string previousVisitDate, std::string referringDoctor)
    : PatientRecord(std::move(id), std::move(name), age, std::move(contact)),
      previousVisitDate_(utils::Validator::trim(previousVisitDate)),
      referringDoctor_(utils::Validator::trim(referringDoctor)) {
    if (!previousVisitDate_.empty()) utils::Validator::validateDate(previousVisitDate_);
}

std::string FollowUpPatient::getSummary() const {
    std::string s = getName() + " (" + std::to_string(getAge()) + ") - follow-up";
    if (!previousVisitDate_.empty()) s += " of visit on " + previousVisitDate_;
    if (!referringDoctor_.empty()) s += ", referred by " + referringDoctor_;
    return s;
}

std::vector<std::string> FollowUpPatient::getExtraFieldLabels() const {
    return {"Previous Visit (YYYY-MM-DD)", "Referring Doctor"};
}

std::vector<std::string> FollowUpPatient::getExtraFields() const { return {previousVisitDate_, referringDoctor_}; }

std::unique_ptr<PatientRecord> FollowUpPatient::clone() const { return std::make_unique<FollowUpPatient>(*this); }

void FollowUpPatient::scheduleAppointment(const Appointment& appointment) {
    // Type-specific rule: a follow-up must come after the visit it reviews.
    if (!previousVisitDate_.empty() && appointment.getDate() <= previousVisitDate_) {
        throw utils::ValidationException("A follow-up appointment must be after the previous visit date (" +
                                         previousVisitDate_ + ").");
    }
    addAppointmentChecked(appointment, maxPendingAppointments());
}

bool FollowUpPatient::cancelAppointment(const std::string& appointmentId) { return markCancelled(appointmentId); }

} // namespace scapms::model
