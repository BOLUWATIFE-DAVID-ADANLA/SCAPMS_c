#pragma once

#include "../model/ClinicDatabase.h"

#include <string>
#include <vector>

namespace scapms::controller {

struct AttendanceRow {
    std::string patientId, name, category;
    int completed = 0;
    int total = 0;
};

struct PendingRow {
    std::string patientId, name, category, appointmentId, dateTime, reason;
};

struct CategoryRow {
    std::string category;
    int count = 0;
    double percent = 0.0;
};

struct DailySummary {
    std::string date;
    int total = 0, completed = 0, pending = 0, cancelled = 0;
};

struct AgeGroupRow {
    std::string group;
    int count = 0;
};

// Builds the clinic reports from the database. Ordering is done with
// hand-written insertion sorts so no library sort is used.
class ReportGenerator {
public:
    static std::vector<AttendanceRow> mostFrequentlyAttended(const model::ClinicDatabase& db, int topN = 10);
    static std::vector<PendingRow> pendingAppointments(const model::ClinicDatabase& db);
    static std::vector<CategoryRow> categoryDistribution(const model::ClinicDatabase& db);
    static DailySummary dailySummary(const model::ClinicDatabase& db, const std::string& date);
    static std::vector<AgeGroupRow> ageGroups(const model::ClinicDatabase& db);
};

} // namespace scapms::controller
