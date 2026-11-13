#include "FileHandler.h"
#include "ClinicException.h"
#include "Validator.h"
#include "../model/PatientFactory.h"

#include <cstdio>
#include <fstream>
#include <sstream>

namespace scapms::utils {

using model::Appointment;
using model::ClinicDatabase;

std::string FileHandler::escape(const std::string& field) {
    std::string out;
    out.reserve(field.size());
    for (char c : field) {
        switch (c) {
            case '\\': out += "\\\\"; break;
            case '|': out += "\\p"; break;
            case '\n': out += "\\n"; break;
            case '\r': break;
            default: out += c;
        }
    }
    return out;
}

std::vector<std::string> FileHandler::splitEscaped(const std::string& line) {
    std::vector<std::string> fields(1);
    for (std::size_t i = 0; i < line.size(); ++i) {
        const char c = line[i];
        if (c == '\\' && i + 1 < line.size()) {
            const char n = line[++i];
            if (n == 'p') fields.back() += '|';
            else if (n == 'n') fields.back() += '\n';
            else fields.back() += n;
        } else if (c == '|') {
            fields.emplace_back();
        } else if (c != '\r') {
            fields.back() += c;
        }
    }
    return fields;
}

void FileHandler::save(const std::string& path, const ClinicDatabase& db) {
    const std::string tmp = path + ".tmp";
    {
        std::ofstream out(tmp, std::ios::trunc);
        if (!out) throw FileException("Cannot open '" + path + "' for writing. Check the folder exists and is writable.");

        out << "#SCAPMS|1\n";
        for (const auto& p : db.patients()) {
            const auto extras = p->getExtraFields();
            out << "P|" << escape(model::categoryToString(p->getCategory())) << '|' << escape(p->getId()) << '|'
                << escape(p->getName()) << '|' << p->getAge() << '|' << escape(p->getContact()) << '|'
                << escape(extras.size() > 0 ? extras[0] : "") << '|' << escape(extras.size() > 1 ? extras[1] : "")
                << '\n';
        }
        for (const auto& p : db.patients()) {
            for (const auto& a : p->getAppointments()) {
                out << "A|" << escape(a.getId()) << '|' << escape(p->getId()) << '|' << a.getDate() << '|'
                    << a.getTime() << '|' << escape(a.getReason()) << '|' << model::statusToString(a.getStatus())
                    << '\n';
            }
        }
        for (const auto& q : db.queueSnapshot()) out << "Q|" << escape(q.patientId) << '|' << q.joinedAt << '\n';

        out.flush();
        if (!out) throw FileException("Writing to '" + path + "' failed (disk full or permission denied).");
    }
    std::remove(path.c_str());
    if (std::rename(tmp.c_str(), path.c_str()) != 0) {
        std::remove(tmp.c_str());
        throw FileException("Could not replace '" + path + "' with the saved data.");
    }
}

ClinicDatabase FileHandler::load(const std::string& path) {
    std::ifstream in(path);
    if (!in) throw FileException("Cannot open '" + path + "'. The file may not exist or is not readable.");

    ClinicDatabase db;
    std::string line;
    int lineNo = 0;
    bool headerSeen = false;

    while (std::getline(in, line)) {
        ++lineNo;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (Validator::trim(line).empty()) continue;

        if (!headerSeen) {
            if (line.rfind("#SCAPMS", 0) != 0)
                throw FileException("'" + path + "' is not a SCAPMS data file (missing #SCAPMS header).");
            headerSeen = true;
            continue;
        }
        if (line[0] == '#') continue;  // comment

        try {
            const auto f = splitEscaped(line);
            if (f[0] == "P") {
                if (f.size() < 6) throw ValidationException("patient record has too few fields");
                const auto category = model::categoryFromString(f[1]);
                const int age = Validator::parseAge(f[4]);
                std::vector<std::string> extras;
                if (f.size() > 6) extras.push_back(f[6]);
                if (f.size() > 7) extras.push_back(f[7]);
                db.addPatient(model::PatientFactory::create(category, f[2], f[3], age, f[5], extras));
            } else if (f[0] == "A") {
                if (f.size() < 7) throw ValidationException("appointment record has too few fields");
                model::PatientRecord* p = db.getPatient(f[2]);
                if (p == nullptr) throw ValidationException("appointment refers to unknown patient " + f[2]);
                // Historical appointments are restored as-is (booking rules are not re-applied).
                p->attachAppointment(Appointment(f[1], f[2], f[3], f[4], f[5], model::statusFromString(f[6])));
            } else if (f[0] == "Q") {
                if (f.size() < 3) throw ValidationException("queue record has too few fields");
                db.enqueue(f[1], std::stoll(f[2]));
            } else {
                throw ValidationException("unknown record type '" + f[0] + "'");
            }
        } catch (const ClinicException& e) {
            throw FileException("Error in '" + path + "' at line " + std::to_string(lineNo) + ": " + e.what());
        } catch (const std::exception& e) {
            throw FileException("Error in '" + path + "' at line " + std::to_string(lineNo) +
                                ": malformed value (" + e.what() + ")");
        }
    }
    if (!headerSeen) throw FileException("'" + path + "' is empty.");
    return db;
}

std::string FileHandler::csvQuote(const std::string& field) {
    bool needsQuotes = false;
    for (char c : field) {
        if (c == ',' || c == '"' || c == '\n') { needsQuotes = true; break; }
    }
    if (!needsQuotes) return field;
    std::string out = "\"";
    for (char c : field) {
        if (c == '"') out += "\"\"";
        else out += c;
    }
    return out + "\"";
}

std::vector<std::string> FileHandler::parseCsvLine(const std::string& line) {
    std::vector<std::string> fields(1);
    bool inQuotes = false;
    for (std::size_t i = 0; i < line.size(); ++i) {
        const char c = line[i];
        if (inQuotes) {
            if (c == '"' && i + 1 < line.size() && line[i + 1] == '"') { fields.back() += '"'; ++i; }
            else if (c == '"') inQuotes = false;
            else fields.back() += c;
        } else if (c == '"') {
            inQuotes = true;
        } else if (c == ',') {
            fields.emplace_back();
        } else if (c != '\r') {
            fields.back() += c;
        }
    }
    return fields;
}

void FileHandler::exportCsv(const std::string& path, const ClinicDatabase& db) {
    std::ofstream out(path, std::ios::trunc);
    if (!out) throw FileException("Cannot open '" + path + "' for writing.");
    out << "PatientID,Name,Age,Contact,Category,Priority,Extra1,Extra2,TotalAppointments,Pending,Completed\n";
    for (const auto& p : db.patients()) {
        const auto e = p->getExtraFields();
        out << csvQuote(p->getId()) << ',' << csvQuote(p->getName()) << ',' << p->getAge() << ','
            << csvQuote(p->getContact()) << ',' << csvQuote(p->getCategoryName()) << ','
            << csvQuote(p->getPriorityLabel()) << ',' << csvQuote(e.size() > 0 ? e[0] : "") << ','
            << csvQuote(e.size() > 1 ? e[1] : "") << ',' << p->getAppointments().size() << ','
            << p->countAppointments(model::AppointmentStatus::Pending) << ','
            << p->countAppointments(model::AppointmentStatus::Completed) << '\n';
    }
    if (!out) throw FileException("Writing to '" + path + "' failed.");
}

FileHandler::ImportReport FileHandler::importCsv(const std::string& path, ClinicDatabase& db) {
    std::ifstream in(path);
    if (!in) throw FileException("Cannot open '" + path + "'.");

    ImportReport report;
    std::string line;
    int lineNo = 0;
    while (std::getline(in, line)) {
        ++lineNo;
        if (lineNo == 1 && Validator::startsWithIgnoreCase(line, "PatientID")) continue;  // header row
        if (Validator::trim(line).empty()) continue;
        try {
            const auto f = parseCsvLine(line);
            if (f.size() < 5) throw ValidationException("expected at least 5 columns (ID,Name,Age,Contact,Category)");
            std::vector<std::string> extras;
            if (f.size() > 6) extras.push_back(f[6]);
            if (f.size() > 7) extras.push_back(f[7]);
            db.addPatient(model::PatientFactory::create(model::categoryFromString(f[4]), f[0], f[1],
                                                        Validator::parseAge(f[2]), f[3], extras));
            ++report.imported;
        } catch (const std::exception& e) {
            ++report.skipped;
            report.problems.push_back("Line " + std::to_string(lineNo) + ": " + e.what());
        }
    }
    return report;
}

} // namespace scapms::utils
