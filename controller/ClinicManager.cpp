#include "ClinicManager.h"
#include "DemoData.h"
#include "../model/PatientFactory.h"
#include "../utils/ClinicException.h"
#include "../utils/DateUtils.h"
#include "../utils/Validator.h"

#include <utility>

namespace scapms::controller {

using model::Appointment;
using model::PatientRecord;
namespace V = utils::Validator;

ClinicManager::ClinicManager() : appointments_(db_, appointmentIds_) {}

// ===================================================================== patients

std::unique_ptr<PatientRecord> ClinicManager::buildPatient(const PatientInput& in, const std::string& id) const {
    // Validate in a fixed order so the first problem reported is the most obvious one.
    V::validatePatientId(id);
    V::validateName(in.name);
    const int age = V::parseAge(in.age);
    V::validateContact(in.contact);
    if (in.category == model::PatientCategory::FollowUp && !V::trim(in.extra1).empty()) {
        V::validateDate(V::trim(in.extra1));
        if (V::trim(in.extra1) > utils::DateUtils::today())
            throw utils::ValidationException("Previous visit date cannot be in the future.");
    }
    return model::PatientFactory::create(in.category, id, in.name, age, in.contact, {in.extra1, in.extra2});
}

PatientRecord& ClinicManager::registerPatient(const PatientInput& input) {
    const std::string id = V::trim(input.id).empty() ? patientIds_.peek() : V::trim(input.id);
    if (db_.containsId(id)) throw utils::DuplicateIdException(id);

    auto patient = buildPatient(input, id);
    PatientRecord* raw = patient.get();
    db_.addPatient(std::move(patient));
    patientIds_.observe(id);  // keeps auto IDs ahead of manual ones
    db_.touch(raw);
    dirty_ = true;
    return *raw;
}

void ClinicManager::updatePatient(const std::string& patientId, const PatientInput& input) {
    PatientRecord* existing = db_.getPatient(patientId);
    if (existing == nullptr) throw utils::NotFoundException("Patient " + patientId + " was not found.");

    const std::string newId = V::trim(input.id).empty() ? patientId : V::trim(input.id);
    if (newId != patientId && db_.containsId(newId)) throw utils::DuplicateIdException(newId);

    // Build the updated record (may be a different subclass if the type changed),
    // carrying the appointment history across.
    auto updated = buildPatient(input, newId);
    updated->copyAppointmentsFrom(*existing);

    const int index = db_.indexOfPatient(patientId);
    std::unique_ptr<PatientRecord> old = db_.replacePatient(patientId, std::move(updated));
    patientIds_.observe(newId);
    db_.touch(db_.getPatient(newId));

    UndoAction action;
    action.type = UndoType::ModifyPatient;
    action.description = "Modify patient " + old->getId() + " (" + old->getName() + ")";
    action.snapshot = std::shared_ptr<const PatientRecord>(std::move(old));
    action.currentPatientId = newId;
    action.originalIndex = index;
    pushUndo(std::move(action));
}

void ClinicManager::deletePatient(const std::string& patientId) {
    const int index = db_.indexOfPatient(patientId);
    if (index < 0) throw utils::NotFoundException("Patient " + patientId + " was not found.");

    UndoAction action;
    action.type = UndoType::DeletePatient;
    action.originalIndex = index;
    for (const auto& q : db_.queueSnapshot()) {
        if (q.patientId == patientId) { action.wasQueued = true; action.queuedAt = q.joinedAt; }
    }
    db_.removeFromQueue(patientId);

    std::unique_ptr<PatientRecord> removed = db_.removePatient(patientId);
    action.description = "Delete patient " + removed->getId() + " (" + removed->getName() + ")";
    action.snapshot = std::shared_ptr<const PatientRecord>(std::move(removed));
    pushUndo(std::move(action));
}

PatientRecord* ClinicManager::findPatient(const std::string& patientId) {
    long long comparisons = 0;
    const int idx = SearchEngine::linearSearchById(db_.patients(), patientId, comparisons);
    if (idx < 0) return nullptr;
    PatientRecord* p = db_.patients()[idx].get();
    db_.touch(p);  // record in the "frequently accessed" array cache
    return p;
}

std::string ClinicManager::processPatient(const PatientRecord& patient) {
    // Each call below dispatches to the subclass implementation at runtime.
    return "[P" + std::to_string(patient.getPriorityLevel()) + " " + patient.getPriorityLabel() + "] " +
           patient.getCategoryName() + ": " + patient.getSummary();
}

std::vector<std::string> ClinicManager::triageOverview() const {
    // Collect base-class pointers, then order by priority level (insertion sort, stable).
    std::vector<const PatientRecord*> list;
    for (const auto& p : db_.patients()) list.push_back(p.get());
    for (std::size_t i = 1; i < list.size(); ++i) {
        const PatientRecord* key = list[i];
        std::size_t j = i;
        while (j > 0 && list[j - 1]->getPriorityLevel() > key->getPriorityLevel()) {
            list[j] = list[j - 1];
            --j;
        }
        list[j] = key;
    }
    std::vector<std::string> out;
    for (const PatientRecord* p : list) out.push_back(processPatient(*p));
    return out;
}

// ===================================================================== appointments

Appointment ClinicManager::scheduleAppointment(const std::string& patientId, const std::string& date,
                                               const std::string& time, const std::string& reason) {
    Appointment a = appointments_.schedule(patientId, date, time, reason);
    db_.touch(db_.getPatient(patientId));
    if (db_.sortedBy() == model::SortField::AppointmentDate) db_.setSortedBy(model::SortField::None);
    dirty_ = true;
    return a;
}

void ClinicManager::cancelAppointment(const std::string& patientId, const std::string& appointmentId) {
    UndoAction action;
    action.type = UndoType::CancelAppointment;
    action.appointment = appointments_.cancel(patientId, appointmentId);
    action.description = "Cancel appointment " + appointmentId + " for " + patientId;
    if (db_.sortedBy() == model::SortField::AppointmentDate) db_.setSortedBy(model::SortField::None);
    pushUndo(std::move(action));
}

void ClinicManager::completeAppointment(const std::string& patientId, const std::string& appointmentId) {
    PatientRecord* p = db_.getPatient(patientId);
    if (p == nullptr) throw utils::NotFoundException("Patient " + patientId + " was not found.");
    const Appointment* a = p->findAppointment(appointmentId);
    if (a == nullptr) throw utils::NotFoundException("Appointment " + appointmentId + " was not found.");

    UndoAction action;
    action.type = UndoType::CompleteAppointment;
    action.appointment = *a;
    appointments_.complete(patientId, appointmentId);
    action.description = "Complete appointment " + appointmentId + " for " + patientId;
    if (db_.sortedBy() == model::SortField::AppointmentDate) db_.setSortedBy(model::SortField::None);
    pushUndo(std::move(action));
}

void ClinicManager::removeAppointment(const std::string& patientId, const std::string& appointmentId) {
    UndoAction action;
    action.type = UndoType::RemoveAppointment;
    action.appointment = appointments_.remove(patientId, appointmentId);
    action.description = "Remove appointment " + appointmentId + " for " + patientId;
    if (db_.sortedBy() == model::SortField::AppointmentDate) db_.setSortedBy(model::SortField::None);
    pushUndo(std::move(action));
}

// ===================================================================== queue

void ClinicManager::addToQueue(const std::string& patientId) {
    db_.enqueue(patientId, utils::DateUtils::nowEpoch());
    dirty_ = true;
}

PatientRecord* ClinicManager::callNextPatient(model::QueueEntry& served) {
    served = db_.dequeue();  // FIFO: the first patient to join is served first
    dirty_ = true;
    PatientRecord* p = db_.getPatient(served.patientId);
    db_.touch(p);
    return p;
}

// ===================================================================== undo

void ClinicManager::pushUndo(UndoAction action) {
    undoStack_.push(std::move(action));
    dirty_ = true;
}

void ClinicManager::clearUndo() { undoStack_ = std::stack<UndoAction>(); }

std::string ClinicManager::undoLast() {
    if (undoStack_.empty()) throw utils::OperationException("There is no administrative operation to undo.");
    UndoAction action = undoStack_.top();  // LIFO: most recent operation first

    switch (action.type) {
        case UndoType::DeletePatient: {
            db_.insertPatient(static_cast<std::size_t>(action.originalIndex), action.snapshot->clone());
            if (action.wasQueued) db_.enqueue(action.snapshot->getId(), action.queuedAt);  // rejoins at the back
            break;
        }
        case UndoType::ModifyPatient: {
            if (action.currentPatientId != action.snapshot->getId() && db_.containsId(action.snapshot->getId()))
                throw utils::OperationException("Cannot undo: ID " + action.snapshot->getId() +
                                                " has since been given to another patient.");
            // Keep any appointments made after the edit, then restore the old details.
            std::unique_ptr<PatientRecord> restored = action.snapshot->clone();
            if (PatientRecord* current = db_.getPatient(action.currentPatientId)) restored->copyAppointmentsFrom(*current);
            db_.replacePatient(action.currentPatientId, std::move(restored));
            break;
        }
        case UndoType::CancelAppointment:
        case UndoType::CompleteAppointment:
        case UndoType::RemoveAppointment:
            appointments_.restore(action.appointment);
            break;
    }

    undoStack_.pop();  // only pop once the reversal has succeeded
    dirty_ = true;
    return action.description;
}

std::vector<std::string> ClinicManager::undoHistory() const {
    std::vector<std::string> out;
    std::stack<UndoAction> copy = undoStack_;  // snapshots are shared_ptr, so copying is cheap
    while (!copy.empty()) {
        out.push_back(copy.top().description);
        copy.pop();
    }
    return out;
}

// ===================================================================== search & sort

SearchResult ClinicManager::search(SearchField field, SearchAlgorithm algorithm, const std::string& query) const {
    return SearchEngine::search(db_.patients(), db_.sortedBy(), field, algorithm, query);
}

SortStats ClinicManager::sort(SortAlgorithm algorithm, model::SortField field, bool ascending) {
    if (field == model::SortField::None) throw utils::ValidationException("Please choose a field to sort by.");
    SortStats stats = SortEngine::sort(db_.patients(), algorithm, field, ascending);
    // Binary search assumes ascending order, so only record ascending sorts.
    db_.setSortedBy(ascending ? field : model::SortField::None);
    return stats;
}

// ===================================================================== persistence

void ClinicManager::resyncIdGenerators() {
    patientIds_.reset();
    appointmentIds_.reset();
    for (const auto& p : db_.patients()) {
        patientIds_.observe(p->getId());
        for (const auto& a : p->getAppointments()) appointmentIds_.observe(a.getId());
    }
}

void ClinicManager::newSession() {
    db_.clear();
    clearUndo();
    resyncIdGenerators();
    currentFile_.clear();
    dirty_ = false;
}

void ClinicManager::save(const std::string& path) {
    utils::FileHandler::save(path, db_);
    currentFile_ = path;
    dirty_ = false;
}

void ClinicManager::load(const std::string& path) {
    model::ClinicDatabase loaded = utils::FileHandler::load(path);  // throws before touching current data
    db_ = std::move(loaded);
    clearUndo();
    resyncIdGenerators();
    currentFile_ = path;
    dirty_ = false;
}

void ClinicManager::exportCsv(const std::string& path) const { utils::FileHandler::exportCsv(path, db_); }

utils::FileHandler::ImportReport ClinicManager::importCsv(const std::string& path) {
    auto report = utils::FileHandler::importCsv(path, db_);
    resyncIdGenerators();
    if (report.imported > 0) dirty_ = true;
    return report;
}

void ClinicManager::loadDemoData() {
    newSession();
    DemoData::populate(db_);
    resyncIdGenerators();
    dirty_ = true;
}

} // namespace scapms::controller
