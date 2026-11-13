#pragma once

#include <string>

namespace scapms::model {

enum class AppointmentStatus { Pending, Completed, Cancelled };

std::string statusToString(AppointmentStatus status);
AppointmentStatus statusFromString(const std::string& text);

// A single scheduled visit. Owned by the PatientRecord it belongs to
// (composition): a patient holds one or more appointments.
class Appointment {
public:
    Appointment() = default;
    Appointment(std::string id, std::string patientId, std::string date, std::string time,
                std::string reason, AppointmentStatus status = AppointmentStatus::Pending);

    const std::string& getId() const { return id_; }
    const std::string& getPatientId() const { return patientId_; }
    const std::string& getDate() const { return date_; }
    const std::string& getTime() const { return time_; }
    const std::string& getReason() const { return reason_; }
    AppointmentStatus getStatus() const { return status_; }
    std::string getDateTime() const { return date_ + " " + time_; }

    bool isPending() const { return status_ == AppointmentStatus::Pending; }

    void setStatus(AppointmentStatus status) { status_ = status; }
    void setPatientId(const std::string& patientId) { patientId_ = patientId; }
    void setReason(const std::string& reason) { reason_ = reason; }

private:
    std::string id_;
    std::string patientId_;
    std::string date_;   // YYYY-MM-DD
    std::string time_;   // HH:MM
    std::string reason_;
    AppointmentStatus status_ = AppointmentStatus::Pending;
};

} // namespace scapms::model
