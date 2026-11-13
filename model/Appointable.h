#pragma once

#include "Appointment.h"

#include <string>

namespace scapms::model {

// Interface-like abstract class: only pure virtual functions, no data.
// Patient types that can be booked for appointments implement it.
class Appointable {
public:
    virtual ~Appointable() = default;

    // Adds the appointment after applying the patient type's booking rules.
    // Throws ValidationException when a rule is violated.
    virtual void scheduleAppointment(const Appointment& appointment) = 0;

    // Marks a pending appointment as cancelled. Returns false if not found or not pending.
    virtual bool cancelAppointment(const std::string& appointmentId) = 0;

    // Booking limit for pending appointments of this patient type.
    virtual int maxPendingAppointments() const = 0;
};

} // namespace scapms::model
