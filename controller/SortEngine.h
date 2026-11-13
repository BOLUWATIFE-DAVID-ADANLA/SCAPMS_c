#pragma once

#include "../model/ClinicDatabase.h"

#include <string>

namespace scapms::controller {

enum class SortAlgorithm { Selection, Insertion, Merge, Quick };

struct SortStats {
    long long comparisons = 0;
    long long moves = 0;   // swaps or element writes
    double microseconds = 0.0;
};

// Hand-written sorting algorithms (no std::sort / std::stable_sort).
// They reorder the patient vector in place according to a chosen field.
class SortEngine {
public:
    using PatientList = model::ClinicDatabase::PatientList;

    static SortStats sort(PatientList& patients, SortAlgorithm algorithm, model::SortField field,
                          bool ascending = true);

    // Three-way comparison: negative if a < b, 0 if equal, positive if a > b.
    static int compare(const model::PatientRecord& a, const model::PatientRecord& b, model::SortField field);
    // Natural ordering for IDs so that P2 < P10 (digits compared as numbers).
    static int compareIds(const std::string& a, const std::string& b);

    static std::string algorithmName(SortAlgorithm algorithm);
    static std::string complexity(SortAlgorithm algorithm);
    // Explains why the algorithm suits the current data size/state.
    static std::string recommendation(SortAlgorithm algorithm, std::size_t n, bool alreadySortedBySameField);
    static SortAlgorithm suggest(std::size_t n, bool nearlySorted);

private:
    struct Context {
        model::SortField field;
        bool ascending;
        SortStats stats;
        int cmp(const model::PatientRecord& a, const model::PatientRecord& b);
    };

    static void selectionSort(PatientList& v, Context& ctx);
    static void insertionSort(PatientList& v, Context& ctx);
    static void mergeSort(PatientList& v, PatientList& buffer, int left, int right, Context& ctx);
    static void merge(PatientList& v, PatientList& buffer, int left, int mid, int right, Context& ctx);
    static void quickSort(PatientList& v, int low, int high, Context& ctx);
    static int partition(PatientList& v, int low, int high, Context& ctx);
};

} // namespace scapms::controller
