#pragma once

#include "Appointable.h"
#include "PatientRecord.h"

namespace scapms::model {

// Walk-in / routine patient. Normal priority, up to 3 pending appointments.
class GeneralPatient : public PatientRecord, public Appointable {
public:
    GeneralPatient(std::string id, std::string name, int age, std::string contact,
                   std::string department = "General Medicine", std::string insuranceId = "");

    PatientCategory getCategory() const override { return PatientCategory::General; }
    std::string getCategoryName() const override { return "General"; }
    int getPriorityLevel() const override { return 2; }
    std::string getPriorityLabel() const override { return "Normal"; }
    std::string getSummary() const override;
    std::vector<std::string> getExtraFieldLabels() const override;
    std::vector<std::string> getExtraFields() const override;
    std::unique_ptr<PatientRecord> clone() const override;

    void scheduleAppointment(const Appointment& appointment) override;
    bool cancelAppointment(const std::string& appointmentId) override;
    int maxPendingAppointments() const override { return 3; }

    const std::string& getDepartment() const { return department_; }
    const std::string& getInsuranceId() const { return insuranceId_; }

private:
    std::string department_;
    std::string insuranceId_;
};

} // namespace scapms::model
