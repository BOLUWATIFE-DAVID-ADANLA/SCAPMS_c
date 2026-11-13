#pragma once

#include "../model/ClinicDatabase.h"

#include <string>
#include <vector>

namespace scapms::utils {

// Reads and writes clinic data.
//
// Native format (.txt), one record per line, '|' separated, with escaping:
//   #SCAPMS|1
//   P|<category>|<id>|<name>|<age>|<contact>|<extra1>|<extra2>
//   A|<apptId>|<patientId>|<date>|<time>|<reason>|<status>
//   Q|<patientId>|<joinedAtEpoch>
//
// CSV (.csv) export/import covers patient details only.
class FileHandler {
public:
    struct ImportReport {
        int imported = 0;
        int skipped = 0;
        std::vector<std::string> problems;
    };

    // Writes to a temp file first, then replaces the target, so a failed save never corrupts data.
    static void save(const std::string& path, const model::ClinicDatabase& db);
    // Parses into a fresh database; throws FileException (with line number) on any error,
    // leaving the caller's current data untouched.
    static model::ClinicDatabase load(const std::string& path);

    static void exportCsv(const std::string& path, const model::ClinicDatabase& db);
    static ImportReport importCsv(const std::string& path, model::ClinicDatabase& db);

    // Exposed for testing.
    static std::string escape(const std::string& field);
    static std::vector<std::string> splitEscaped(const std::string& line);
    static std::vector<std::string> parseCsvLine(const std::string& line);
    static std::string csvQuote(const std::string& field);
};

} // namespace scapms::utils
