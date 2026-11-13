#include "PatientRecord.h"
#include "../utils/ClinicException.h"
#include "../utils/Validator.h"

#include <utility>

namespace scapms::model {

using utils::ValidationException;
namespace V = utils::Validator;

std::string categoryToString(PatientCategory category) {
    switch (category) {
        case PatientCategory::General: return "General";
        case PatientCategory::Emergency: return "Emergency";
        case PatientCategory::FollowUp: return "Follow-Up";
    }
    return "General";
}

PatientCategory categoryFromString(const std::string& text) {
    const std::string t = V::toLower(V::trim(text));
    if (t == "general") return PatientCategory::General;
    if (t == "emergency") return PatientCategory::Emergency;
    if (t == "follow-up" || t == "followup" || t == "follow up") return PatientCategory::FollowUp;
    throw ValidationException("Unknown patient category: " + text);
}

PatientRecord::PatientRecord(std::string id, std::string name, int age, std::string contact) {
    setId(id);
    setName(name);
    setAge(age);
    setContact(contact);
}

void PatientRecord::setId(const std::string& id) {
    const std::string t = V::trim(id);
    V::validatePatientId(t);
    id_ = t;
}

void PatientRecord::setName(const std::string& name) {
    V::validateName(name);
    name_ = V::trim(name);
}

void PatientRecord::setAge(int age) {
    V::validateAge(age);
    age_ = age;
}

void PatientRecord::setContact(const std::string& contact) {
    V::validateContact(contact);
    contact_ = V::trim(contact);
}

Appointment* PatientRecord::findAppointment(const std::string& appointmentId) {
    for (auto& a : appointments_) {
        if (a.getId() == appointmentId) return &a;
    }
    return nullptr;
}

const Appointment* PatientRecord::findAppointment(const std::string& appointmentId) const {
    for (const auto& a : appointments_) {
        if (a.getId() == appointmentId) return &a;
    }
    return nullptr;
}

void PatientRecord::attachAppointment(const Appointment& appointment) {
    Appointment copy = appointment;
    copy.setPatientId(id_);
    appointments_.push_back(copy);
}

Appointment PatientRecord::removeAppointment(const std::string& appointmentId) {
    for (auto it = appointments_.begin(); it != appointments_.end(); ++it) {
        if (it->getId() == appointmentId) {
            Appointment removed = *it;
            appointments_.erase(it);
            return removed;
        }
    }
    throw utils::NotFoundException("Appointment " + appointmentId + " not found for patient " + id_ + ".");
}

void PatientRecord::copyAppointmentsFrom(const PatientRecord& other) {
    appointments_.clear();
    for (const auto& a : other.appointments_) attachAppointment(a);
}

int PatientRecord::countAppointments(AppointmentStatus status) const {
    int count = 0;
    for (const auto& a : appointments_) {
        if (a.getStatus() == status) ++count;
    }
    return count;
}

std::string PatientRecord::nextAppointmentDateTime() const {
    std::string best;
    for (const auto& a : appointments_) {
        if (!a.isPending()) continue;
        const std::string dt = a.getDateTime();
        if (best.empty() || dt < best) best = dt;
    }
    return best;
}

void PatientRecord::addAppointmentChecked(const Appointment& appointment, int maxPending) {
    if (countAppointments(AppointmentStatus::Pending) >= maxPending) {
        throw ValidationException(getCategoryName() + " patients can have at most " + std::to_string(maxPending) +
                                  " pending appointments. Complete or cancel one first.");
    }
    for (const auto& a : appointments_) {
        if (a.isPending() && a.getDateTime() == appointment.getDateTime())
            throw ValidationException(name_ + " already has an appointment at " + appointment.getDateTime() + ".");
        if (a.getId() == appointment.getId())
            throw ValidationException("Appointment ID already exists: " + appointment.getId());
    }
    attachAppointment(appointment);
}

bool PatientRecord::markCancelled(const std::string& appointmentId) {
    Appointment* a = findAppointment(appointmentId);
    if (a == nullptr || !a->isPending()) return false;
    a->setStatus(AppointmentStatus::Cancelled);
    return true;
}

} // namespace scapms::model
