#pragma once

#include "../model/ClinicDatabase.h"
#include "../utils/IDGenerator.h"

#include <string>
#include <vector>

namespace scapms::controller {

// Flattened appointment row for display (appointment + owning patient details).
struct AppointmentView {
    model::Appointment appointment;
    std::string patientName;
    std::string category;
    int priorityLevel = 2;
};

// Business logic for booking, cancelling and completing appointments.
// Booking goes through the Appointable interface so each patient type
// applies its own rules.
class AppointmentController {
public:
    AppointmentController(model::ClinicDatabase& db, utils::IDGenerator& idGenerator);

    model::Appointment schedule(const std::string& patientId, const std::string& date, const std::string& time,
                                const std::string& reason);
    model::Appointment cancel(const std::string& patientId, const std::string& appointmentId);  // returns old state
    void complete(const std::string& patientId, const std::string& appointmentId);
    model::Appointment remove(const std::string& patientId, const std::string& appointmentId);
    void restore(const model::Appointment& appointment);  // undo helper: puts an appointment back as it was

    // All appointments ordered by date/time (manual insertion sort, stable).
    std::vector<AppointmentView> all() const;
    std::vector<AppointmentView> onDate(const std::string& date) const;
    // Pending appointments from (date,time) onward within the given number of minutes.
    std::vector<AppointmentView> upcoming(const std::string& date, const std::string& time, int withinMinutes) const;

    void setDatabase(model::ClinicDatabase& db) { db_ = &db; }

private:
    model::PatientRecord& requirePatient(const std::string& patientId) const;

    model::ClinicDatabase* db_;
    utils::IDGenerator& ids_;
};

} // namespace scapms::controller
