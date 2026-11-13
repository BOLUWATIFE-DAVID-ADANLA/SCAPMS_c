#include "ClinicDatabase.h"
#include "../utils/ClinicException.h"

#include <utility>

namespace scapms::model {

std::string sortFieldToString(SortField field) {
    switch (field) {
        case SortField::None: return "Unsorted";
        case SortField::Name: return "Patient Name";
        case SortField::Id: return "Patient ID";
        case SortField::Age: return "Age";
        case SortField::AppointmentDate: return "Next Appointment Date";
    }
    return "Unsorted";
}

ClinicDatabase::ClinicDatabase() {
    for (auto& slot : recentPatients_) slot = nullptr;
}

int ClinicDatabase::indexOf(const std::string& id) const {
    for (std::size_t i = 0; i < patients_.size(); ++i) {
        if (patients_[i]->getId() == id) return static_cast<int>(i);
    }
    return -1;
}

void ClinicDatabase::addPatient(std::unique_ptr<PatientRecord> patient) {
    if (!patient) throw utils::ValidationException("Cannot add an empty patient record.");
    if (containsId(patient->getId())) throw utils::DuplicateIdException(patient->getId());
    patients_.push_back(std::move(patient));
    sortedBy_ = SortField::None;  // appending breaks any existing order
}

void ClinicDatabase::insertPatient(std::size_t index, std::unique_ptr<PatientRecord> patient) {
    if (!patient) throw utils::ValidationException("Cannot add an empty patient record.");
    if (containsId(patient->getId())) throw utils::DuplicateIdException(patient->getId());
    if (index > patients_.size()) index = patients_.size();
    patients_.insert(patients_.begin() + static_cast<long>(index), std::move(patient));
    sortedBy_ = SortField::None;
}

std::unique_ptr<PatientRecord> ClinicDatabase::removePatient(const std::string& id) {
    const int idx = indexOf(id);
    if (idx < 0) throw utils::NotFoundException("Patient " + id + " was not found.");
    std::unique_ptr<PatientRecord> removed = std::move(patients_[idx]);
    patients_.erase(patients_.begin() + idx);  // erase keeps the remaining order intact
    invalidateCache(removed.get());
    return removed;
}

std::unique_ptr<PatientRecord> ClinicDatabase::replacePatient(const std::string& oldId,
                                                              std::unique_ptr<PatientRecord> updated) {
    const int idx = indexOf(oldId);
    if (idx < 0) throw utils::NotFoundException("Patient " + oldId + " was not found.");
    if (updated->getId() != oldId && containsId(updated->getId()))
        throw utils::DuplicateIdException(updated->getId());

    std::unique_ptr<PatientRecord> old = std::move(patients_[idx]);
    invalidateCache(old.get());
    patients_[idx] = std::move(updated);
    if (old->getId() != patients_[idx]->getId()) renameInQueue(old->getId(), patients_[idx]->getId());
    sortedBy_ = SortField::None;  // edited values may break the order
    return old;
}

PatientRecord* ClinicDatabase::getPatient(const std::string& id) const {
    const int idx = indexOf(id);
    return idx < 0 ? nullptr : patients_[idx].get();
}

bool ClinicDatabase::containsId(const std::string& id) const { return indexOf(id) >= 0; }

void ClinicDatabase::clear() {
    patients_.clear();
    waitingQueue_ = std::queue<QueueEntry>();
    for (auto& slot : recentPatients_) slot = nullptr;
    cacheCount_ = 0;
    sortedBy_ = SortField::None;
}

// ------------------------------------------------------------------ queue

void ClinicDatabase::enqueue(const std::string& patientId, long long joinedAt) {
    if (!containsId(patientId)) throw utils::NotFoundException("Patient " + patientId + " was not found.");
    if (isQueued(patientId)) throw utils::OperationException("Patient " + patientId + " is already in the waiting queue.");
    waitingQueue_.push({patientId, joinedAt});
}

QueueEntry ClinicDatabase::dequeue() {
    if (waitingQueue_.empty()) throw utils::OperationException("The waiting queue is empty.");
    QueueEntry front = waitingQueue_.front();
    waitingQueue_.pop();
    return front;
}

bool ClinicDatabase::isQueued(const std::string& patientId) const {
    std::queue<QueueEntry> copy = waitingQueue_;  // std::queue has no iterators, so walk a copy
    while (!copy.empty()) {
        if (copy.front().patientId == patientId) return true;
        copy.pop();
    }
    return false;
}

bool ClinicDatabase::removeFromQueue(const std::string& patientId) {
    bool removed = false;
    std::queue<QueueEntry> rebuilt;
    while (!waitingQueue_.empty()) {
        if (waitingQueue_.front().patientId == patientId) removed = true;
        else rebuilt.push(waitingQueue_.front());
        waitingQueue_.pop();
    }
    waitingQueue_ = std::move(rebuilt);  // relative order of the others is preserved
    return removed;
}

void ClinicDatabase::renameInQueue(const std::string& oldId, const std::string& newId) {
    std::queue<QueueEntry> rebuilt;
    while (!waitingQueue_.empty()) {
        QueueEntry e = waitingQueue_.front();
        waitingQueue_.pop();
        if (e.patientId == oldId) e.patientId = newId;
        rebuilt.push(e);
    }
    waitingQueue_ = std::move(rebuilt);
}

std::vector<QueueEntry> ClinicDatabase::queueSnapshot() const {
    std::vector<QueueEntry> out;
    std::queue<QueueEntry> copy = waitingQueue_;
    while (!copy.empty()) {
        out.push_back(copy.front());
        copy.pop();
    }
    return out;
}

// ------------------------------------------------------------------ cache

void ClinicDatabase::touch(PatientRecord* patient) {
    if (patient == nullptr) return;

    // Find existing slot; if present, shift everything before it right by one.
    int pos = -1;
    for (int i = 0; i < cacheCount_; ++i) {
        if (recentPatients_[i] == patient) { pos = i; break; }
    }
    if (pos < 0) {
        // Not cached: drop the least recently used entry when full.
        pos = cacheCount_ < CACHE_SIZE ? cacheCount_++ : CACHE_SIZE - 1;
    }
    for (int i = pos; i > 0; --i) recentPatients_[i] = recentPatients_[i - 1];
    recentPatients_[0] = patient;
}

void ClinicDatabase::invalidateCache(const PatientRecord* patient) {
    int write = 0;
    for (int read = 0; read < cacheCount_; ++read) {
        if (recentPatients_[read] != patient) recentPatients_[write++] = recentPatients_[read];
    }
    for (int i = write; i < cacheCount_; ++i) recentPatients_[i] = nullptr;
    cacheCount_ = write;
}

std::vector<PatientRecord*> ClinicDatabase::cachedPatients() const {
    return std::vector<PatientRecord*>(recentPatients_, recentPatients_ + cacheCount_);
}

} // namespace scapms::model
