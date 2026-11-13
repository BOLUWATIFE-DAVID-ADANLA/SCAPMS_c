#pragma once

#include "PatientRecord.h"

#include <memory>
#include <queue>
#include <string>
#include <vector>

namespace scapms::model {

// Field the patient vector is currently ordered by. Search uses this to
// decide whether binary search is valid (sorted) or linear search is needed.
enum class SortField { None, Name, Id, Age, AppointmentDate };

std::string sortFieldToString(SortField field);

struct QueueEntry {
    std::string patientId;
    long long joinedAt = 0;  // epoch seconds
};

// Owns all clinic data (encapsulation + composition):
//  - std::vector of patient records (each patient owns its appointments)
//  - std::queue of patients waiting for consultation (FIFO)
//  - fixed-size array cache of the most recently accessed patients
class ClinicDatabase {
public:
    static constexpr int CACHE_SIZE = 10;
    using PatientList = std::vector<std::unique_ptr<PatientRecord>>;

    ClinicDatabase();
    ClinicDatabase(ClinicDatabase&&) = default;
    ClinicDatabase& operator=(ClinicDatabase&&) = default;
    ClinicDatabase(const ClinicDatabase&) = delete;
    ClinicDatabase& operator=(const ClinicDatabase&) = delete;

    // ---- Patients (std::vector) ----
    void addPatient(std::unique_ptr<PatientRecord> patient);              // throws DuplicateIdException
    void insertPatient(std::size_t index, std::unique_ptr<PatientRecord> patient);  // used by undo
    int indexOfPatient(const std::string& id) const { return indexOf(id); }
    std::unique_ptr<PatientRecord> removePatient(const std::string& id);  // throws NotFoundException
    // Swaps the record with oldId for a new one in the same slot; returns the old record.
    std::unique_ptr<PatientRecord> replacePatient(const std::string& oldId, std::unique_ptr<PatientRecord> updated);
    PatientRecord* getPatient(const std::string& id) const;  // nullptr if absent
    bool containsId(const std::string& id) const;
    PatientList& patients() { return patients_; }
    const PatientList& patients() const { return patients_; }
    std::size_t size() const { return patients_.size(); }
    void clear();

    // ---- Waiting queue (std::queue, FIFO) ----
    void enqueue(const std::string& patientId, long long joinedAt);
    QueueEntry dequeue();
    bool isQueued(const std::string& patientId) const;
    bool removeFromQueue(const std::string& patientId);
    void renameInQueue(const std::string& oldId, const std::string& newId);
    std::vector<QueueEntry> queueSnapshot() const;
    std::size_t queueSize() const { return waitingQueue_.size(); }

    // ---- Recently accessed cache (fixed-size array, most recent first) ----
    void touch(PatientRecord* patient);
    void invalidateCache(const PatientRecord* patient);
    std::vector<PatientRecord*> cachedPatients() const;
    int cacheCount() const { return cacheCount_; }

    // ---- Sort state ----
    SortField sortedBy() const { return sortedBy_; }
    void setSortedBy(SortField field) { sortedBy_ = field; }

private:
    int indexOf(const std::string& id) const;

    PatientList patients_;
    std::queue<QueueEntry> waitingQueue_;
    PatientRecord* recentPatients_[CACHE_SIZE];
    int cacheCount_ = 0;
    SortField sortedBy_ = SortField::None;
};

} // namespace scapms::model
