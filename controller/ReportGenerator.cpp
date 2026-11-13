#include "ReportGenerator.h"

namespace scapms::controller {

using model::AppointmentStatus;

std::vector<AttendanceRow> ReportGenerator::mostFrequentlyAttended(const model::ClinicDatabase& db, int topN) {
    std::vector<AttendanceRow> rows;
    for (const auto& p : db.patients()) {
        const int completed = p->countAppointments(AppointmentStatus::Completed);
        if (completed == 0) continue;
        rows.push_back({p->getId(), p->getName(), p->getCategoryName(), completed,
                        static_cast<int>(p->getAppointments().size())});
    }
    // Insertion sort, descending by completed visits, then by name.
    for (std::size_t i = 1; i < rows.size(); ++i) {
        AttendanceRow key = rows[i];
        std::size_t j = i;
        while (j > 0 && (rows[j - 1].completed < key.completed ||
                         (rows[j - 1].completed == key.completed && rows[j - 1].name > key.name))) {
            rows[j] = rows[j - 1];
            --j;
        }
        rows[j] = key;
    }
    if (topN > 0 && static_cast<int>(rows.size()) > topN) rows.resize(topN);
    return rows;
}

std::vector<PendingRow> ReportGenerator::pendingAppointments(const model::ClinicDatabase& db) {
    std::vector<PendingRow> rows;
    for (const auto& p : db.patients()) {
        for (const auto& a : p->getAppointments()) {
            if (a.isPending())
                rows.push_back({p->getId(), p->getName(), p->getCategoryName(), a.getId(), a.getDateTime(),
                                a.getReason()});
        }
    }
    for (std::size_t i = 1; i < rows.size(); ++i) {
        PendingRow key = rows[i];
        std::size_t j = i;
        while (j > 0 && rows[j - 1].dateTime > key.dateTime) {
            rows[j] = rows[j - 1];
            --j;
        }
        rows[j] = key;
    }
    return rows;
}

std::vector<CategoryRow> ReportGenerator::categoryDistribution(const model::ClinicDatabase& db) {
    int counts[3] = {0, 0, 0};
    for (const auto& p : db.patients()) counts[static_cast<int>(p->getCategory())]++;
    const double total = static_cast<double>(db.size());
    std::vector<CategoryRow> rows = {
        {"General Patients", counts[static_cast<int>(model::PatientCategory::General)], 0.0},
        {"Emergency Patients", counts[static_cast<int>(model::PatientCategory::Emergency)], 0.0},
        {"Follow-Up Patients", counts[static_cast<int>(model::PatientCategory::FollowUp)], 0.0},
    };
    for (auto& r : rows) r.percent = total > 0 ? 100.0 * r.count / total : 0.0;
    return rows;
}

DailySummary ReportGenerator::dailySummary(const model::ClinicDatabase& db, const std::string& date) {
    DailySummary s;
    s.date = date;
    for (const auto& p : db.patients()) {
        for (const auto& a : p->getAppointments()) {
            if (a.getDate() != date) continue;
            ++s.total;
            switch (a.getStatus()) {
                case AppointmentStatus::Completed: ++s.completed; break;
                case AppointmentStatus::Pending: ++s.pending; break;
                case AppointmentStatus::Cancelled: ++s.cancelled; break;
            }
        }
    }
    return s;
}

std::vector<AgeGroupRow> ReportGenerator::ageGroups(const model::ClinicDatabase& db) {
    std::vector<AgeGroupRow> rows = {{"0 - 12 (Children)", 0}, {"13 - 17 (Teens)", 0},
                                     {"18 - 35 (Young adults)", 0}, {"36 - 59 (Adults)", 0},
                                     {"60+ (Seniors)", 0}};
    for (const auto& p : db.patients()) {
        const int age = p->getAge();
        if (age <= 12) rows[0].count++;
        else if (age <= 17) rows[1].count++;
        else if (age <= 35) rows[2].count++;
        else if (age <= 59) rows[3].count++;
        else rows[4].count++;
    }
    return rows;
}

} // namespace scapms::controller
