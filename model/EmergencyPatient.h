#pragma once

#include "Appointable.h"
#include "PatientRecord.h"

namespace scapms::model {

// Urgent case. Always high priority; carries a triage severity (1-5)
// and a presenting condition. Allowed more pending appointments (5).
class EmergencyPatient : public PatientRecord, public Appointable {
public:
    EmergencyPatient(std::string id, std::string name, int age, std::string contact,
                     int severity = 3, std::string condition = "Unspecified");

    PatientCategory getCategory() const override { return PatientCategory::Emergency; }
    std::string getCategoryName() const override { return "Emergency"; }
    int getPriorityLevel() const override { return 1; }
    std::string getPriorityLabel() const override { return "High Priority"; }
    std::string getSummary() const override;
    std::vector<std::string> getExtraFieldLabels() const override;
    std::vector<std::string> getExtraFields() const override;
    std::unique_ptr<PatientRecord> clone() const override;

    void scheduleAppointment(const Appointment& appointment) override;
    bool cancelAppointment(const std::string& appointmentId) override;
    int maxPendingAppointments() const override { return 5; }

    int getSeverity() const { return severity_; }
    const std::string& getCondition() const { return condition_; }
    bool isCritical() const { return severity_ >= 4; }

private:
    int severity_;
    std::string condition_;
};

} // namespace scapms::model
