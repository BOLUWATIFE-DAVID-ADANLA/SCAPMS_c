#pragma once

#include "Appointable.h"
#include "PatientRecord.h"

namespace scapms::model {

// Returning patient reviewing an earlier visit. Appointments must fall
// after the previous visit date, and at most 2 may be pending at once.
class FollowUpPatient : public PatientRecord, public Appointable {
public:
    FollowUpPatient(std::string id, std::string name, int age, std::string contact,
                    std::string previousVisitDate = "", std::string referringDoctor = "");

    PatientCategory getCategory() const override { return PatientCategory::FollowUp; }
    std::string getCategoryName() const override { return "Follow-Up"; }
    int getPriorityLevel() const override { return 3; }
    std::string getPriorityLabel() const override { return "Scheduled"; }
    std::string getSummary() const override;
    std::vector<std::string> getExtraFieldLabels() const override;
    std::vector<std::string> getExtraFields() const override;
    std::unique_ptr<PatientRecord> clone() const override;

    void scheduleAppointment(const Appointment& appointment) override;
    bool cancelAppointment(const std::string& appointmentId) override;
    int maxPendingAppointments() const override { return 2; }

    const std::string& getPreviousVisitDate() const { return previousVisitDate_; }
    const std::string& getReferringDoctor() const { return referringDoctor_; }

private:
    std::string previousVisitDate_;
    std::string referringDoctor_;
};

} // namespace scapms::model
