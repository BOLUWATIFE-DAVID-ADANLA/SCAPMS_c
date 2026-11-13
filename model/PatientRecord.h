#pragma once

#include "Appointment.h"

#include <memory>
#include <string>
#include <vector>

namespace scapms::model {

enum class PatientCategory { General, Emergency, FollowUp };

std::string categoryToString(PatientCategory category);
PatientCategory categoryFromString(const std::string& text);

// Abstract base class holding the data common to every patient.
// It cannot be instantiated: subclasses must implement the pure virtual functions.
class PatientRecord {
public:
    PatientRecord(std::string id, std::string name, int age, std::string contact);
    virtual ~PatientRecord() = default;

    // ---- Pure virtual interface (runtime polymorphism) ----
    virtual PatientCategory getCategory() const = 0;
    virtual std::string getCategoryName() const = 0;
    virtual int getPriorityLevel() const = 0;          // 1 = highest priority
    virtual std::string getPriorityLabel() const = 0;  // "High Priority", "Normal", "Scheduled"
    virtual std::string getSummary() const = 0;        // type-specific description
    virtual std::vector<std::string> getExtraFieldLabels() const = 0;
    virtual std::vector<std::string> getExtraFields() const = 0;
    virtual std::unique_ptr<PatientRecord> clone() const = 0;  // deep copy, used by undo

    // ---- Encapsulated common data ----
    const std::string& getId() const { return id_; }
    const std::string& getName() const { return name_; }
    int getAge() const { return age_; }
    const std::string& getContact() const { return contact_; }

    void setId(const std::string& id);
    void setName(const std::string& name);
    void setAge(int age);
    void setContact(const std::string& contact);

    // ---- Composition: appointments owned by this patient ----
    const std::vector<Appointment>& getAppointments() const { return appointments_; }
    Appointment* findAppointment(const std::string& appointmentId);
    const Appointment* findAppointment(const std::string& appointmentId) const;
    void attachAppointment(const Appointment& appointment);  // raw insert, used when loading/restoring
    Appointment removeAppointment(const std::string& appointmentId);
    void copyAppointmentsFrom(const PatientRecord& other);
    int countAppointments(AppointmentStatus status) const;
    std::string nextAppointmentDateTime() const;  // earliest pending, "" when none

protected:
    PatientRecord(const PatientRecord&) = default;
    PatientRecord& operator=(const PatientRecord&) = default;

    // Shared booking logic reused by the Appointable implementations in subclasses.
    void addAppointmentChecked(const Appointment& appointment, int maxPending);
    bool markCancelled(const std::string& appointmentId);

private:
    std::string id_;
    std::string name_;
    int age_ = 0;
    std::string contact_;
    std::vector<Appointment> appointments_;
};

} // namespace scapms::model
