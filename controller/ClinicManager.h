#pragma once

#include "AppointmentController.h"
#include "SearchEngine.h"
#include "SortEngine.h"
#include "../model/ClinicDatabase.h"
#include "../utils/FileHandler.h"
#include "../utils/IDGenerator.h"

#include <memory>
#include <stack>
#include <string>
#include <vector>

namespace scapms::controller {

// Raw form values for creating or editing a patient. Parsed and validated here,
// so the GUI never builds model objects directly.
struct PatientInput {
    model::PatientCategory category = model::PatientCategory::General;
    std::string id;       // empty = auto-generate
    std::string name;
    std::string age;
    std::string contact;
    std::string extra1;
    std::string extra2;
};

enum class UndoType { DeletePatient, ModifyPatient, CancelAppointment, CompleteAppointment, RemoveAppointment };

// One reversible administrative operation, stored on the undo stack.
struct UndoAction {
    UndoType type;
    std::string description;
    std::shared_ptr<const model::PatientRecord> snapshot;  // patient state before the operation
    std::string currentPatientId;                           // ID the patient has now (modify may rename)
    int originalIndex = -1;                                 // position in the vector (restore in place)
    model::Appointment appointment;                         // appointment state before the operation
    bool wasQueued = false;
    long long queuedAt = 0;
};

// Facade over the model: the single entry point the GUI talks to.
// Coordinates the database, appointment controller, search/sort engines,
// the undo stack and file persistence.
class ClinicManager {
public:
    ClinicManager();

    model::ClinicDatabase& database() { return db_; }
    const model::ClinicDatabase& database() const { return db_; }
    AppointmentController& appointments() { return appointments_; }

    // ---- Patients ----
    std::string suggestPatientId() const { return patientIds_.peek(); }
    model::PatientRecord& registerPatient(const PatientInput& input);
    void updatePatient(const std::string& patientId, const PatientInput& input);  // undoable
    void deletePatient(const std::string& patientId);                             // undoable
    model::PatientRecord* findPatient(const std::string& patientId);              // linear search + cache

    // Runtime polymorphism: works on any PatientRecord through a base-class reference.
    static std::string processPatient(const model::PatientRecord& patient);
    std::vector<std::string> triageOverview() const;

    // ---- Appointments ----
    model::Appointment scheduleAppointment(const std::string& patientId, const std::string& date,
                                           const std::string& time, const std::string& reason);
    void cancelAppointment(const std::string& patientId, const std::string& appointmentId);    // undoable
    void completeAppointment(const std::string& patientId, const std::string& appointmentId);  // undoable
    void removeAppointment(const std::string& patientId, const std::string& appointmentId);    // undoable

    // ---- Waiting queue ----
    void addToQueue(const std::string& patientId);
    model::PatientRecord* callNextPatient(model::QueueEntry& served);

    // ---- Undo (std::stack) ----
    bool canUndo() const { return !undoStack_.empty(); }
    std::string undoLast();
    std::vector<std::string> undoHistory() const;  // most recent first

    // ---- Search & sort ----
    SearchResult search(SearchField field, SearchAlgorithm algorithm, const std::string& query) const;
    SortStats sort(SortAlgorithm algorithm, model::SortField field, bool ascending);

    // ---- Persistence ----
    void newSession();
    void save(const std::string& path);
    void load(const std::string& path);
    void exportCsv(const std::string& path) const;
    utils::FileHandler::ImportReport importCsv(const std::string& path);
    void loadDemoData();
    const std::string& currentFile() const { return currentFile_; }
    bool isDirty() const { return dirty_; }

private:
    std::unique_ptr<model::PatientRecord> buildPatient(const PatientInput& input, const std::string& id) const;
    void pushUndo(UndoAction action);
    void resyncIdGenerators();
    void clearUndo();

    model::ClinicDatabase db_;
    utils::IDGenerator patientIds_{"P"};
    utils::IDGenerator appointmentIds_{"A"};
    AppointmentController appointments_;
    std::stack<UndoAction> undoStack_;
    std::string currentFile_;
    bool dirty_ = false;
};

} // namespace scapms::controller
