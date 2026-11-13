#include "AppointmentController.h"
#include "../model/Appointable.h"
#include "../utils/ClinicException.h"
#include "../utils/DateUtils.h"
#include "../utils/Validator.h"

namespace scapms::controller {

using model::Appointment;
using model::AppointmentStatus;
namespace V = utils::Validator;

namespace {
int toMinutes(const std::string& hhmm) { return std::stoi(hhmm.substr(0, 2)) * 60 + std::stoi(hhmm.substr(3, 2)); }
} // namespace

AppointmentController::AppointmentController(model::ClinicDatabase& db, utils::IDGenerator& idGenerator)
    : db_(&db), ids_(idGenerator) {}

model::PatientRecord& AppointmentController::requirePatient(const std::string& patientId) const {
    model::PatientRecord* p = db_->getPatient(patientId);
    if (p == nullptr) throw utils::NotFoundException("Patient " + patientId + " was not found.");
    return *p;
}

Appointment AppointmentController::schedule(const std::string& patientId, const std::string& date,
                                            const std::string& time, const std::string& reason) {
    model::PatientRecord& patient = requirePatient(patientId);

    V::validateDate(date);
    V::validateTime(time);
    const std::string today = utils::DateUtils::today();
    if (date < today) throw utils::ValidationException("Appointment date cannot be in the past.");
    if (date == today && time < utils::DateUtils::currentTime())
        throw utils::ValidationException("Appointment time has already passed today.");
    if (toMinutes(time) < 8 * 60 || toMinutes(time) > 18 * 60)
        throw utils::ValidationException("The clinic books appointments between 08:00 and 18:00.");
    if (V::trim(reason).empty()) throw utils::ValidationException("Please enter a reason for the appointment.");

    // Interface check: only Appointable patient types can be booked.
    auto* appointable = dynamic_cast<model::Appointable*>(&patient);
    if (appointable == nullptr)
        throw utils::OperationException(patient.getCategoryName() + " patients cannot be scheduled for appointments.");

    Appointment appt(ids_.peek(), patientId, date, time, V::trim(reason));
    appointable->scheduleAppointment(appt);  // throws if a type-specific rule fails
    ids_.next();                             // consume the ID only on success
    return appt;
}

Appointment AppointmentController::cancel(const std::string& patientId, const std::string& appointmentId) {
    model::PatientRecord& patient = requirePatient(patientId);
    const Appointment* existing = patient.findAppointment(appointmentId);
    if (existing == nullptr) throw utils::NotFoundException("Appointment " + appointmentId + " was not found.");
    if (!existing->isPending())
        throw utils::OperationException("Only pending appointments can be cancelled (this one is " +
                                        model::statusToString(existing->getStatus()) + ").");
    const Appointment before = *existing;

    auto* appointable = dynamic_cast<model::Appointable*>(&patient);
    if (appointable == nullptr || !appointable->cancelAppointment(appointmentId))
        throw utils::OperationException("Appointment " + appointmentId + " could not be cancelled.");
    return before;
}

void AppointmentController::complete(const std::string& patientId, const std::string& appointmentId) {
    model::PatientRecord& patient = requirePatient(patientId);
    Appointment* a = patient.findAppointment(appointmentId);
    if (a == nullptr) throw utils::NotFoundException("Appointment " + appointmentId + " was not found.");
    if (!a->isPending()) throw utils::OperationException("Only pending appointments can be marked as completed.");
    a->setStatus(AppointmentStatus::Completed);
}

Appointment AppointmentController::remove(const std::string& patientId, const std::string& appointmentId) {
    return requirePatient(patientId).removeAppointment(appointmentId);
}

void AppointmentController::restore(const Appointment& appointment) {
    model::PatientRecord& patient = requirePatient(appointment.getPatientId());
    if (Appointment* existing = patient.findAppointment(appointment.getId())) {
        existing->setStatus(appointment.getStatus());
    } else {
        patient.attachAppointment(appointment);
    }
}

std::vector<AppointmentView> AppointmentController::all() const {
    std::vector<AppointmentView> out;
    for (const auto& p : db_->patients()) {
        for (const auto& a : p->getAppointments()) {
            out.push_back({a, p->getName(), p->getCategoryName(), p->getPriorityLevel()});
        }
    }
    // Insertion sort by date/time (stable, and the per-patient lists are already mostly ordered).
    for (std::size_t i = 1; i < out.size(); ++i) {
        AppointmentView key = out[i];
        std::size_t j = i;
        while (j > 0 && out[j - 1].appointment.getDateTime() > key.appointment.getDateTime()) {
            out[j] = out[j - 1];
            --j;
        }
        out[j] = key;
    }
    return out;
}

std::vector<AppointmentView> AppointmentController::onDate(const std::string& date) const {
    std::vector<AppointmentView> out;
    for (const auto& v : all()) {
        if (v.appointment.getDate() == date) out.push_back(v);
    }
    return out;
}

std::vector<AppointmentView> AppointmentController::upcoming(const std::string& date, const std::string& time,
                                                             int withinMinutes) const {
    std::vector<AppointmentView> out;
    const std::string from = date + " " + time;
    const int nowMin = toMinutes(time);
    for (const auto& v : all()) {
        const Appointment& a = v.appointment;
        if (!a.isPending() || a.getDateTime() < from) continue;
        if (withinMinutes < 0) {
            out.push_back(v);
        } else if (a.getDate() == date && toMinutes(a.getTime()) - nowMin <= withinMinutes) {
            out.push_back(v);
        }
    }
    return out;
}

} // namespace scapms::controller
