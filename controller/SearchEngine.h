#pragma once

#include "../model/ClinicDatabase.h"

#include <string>
#include <vector>

namespace scapms::controller {

enum class SearchField { Id, Name, Category };
enum class SearchAlgorithm { Auto, Linear, Binary, Recursive };

struct SearchResult {
    std::vector<int> indices;   // positions in the patient vector
    std::string algorithmUsed;
    std::string reason;         // why this technique was chosen
    long long comparisons = 0;
    double microseconds = 0.0;
};

// Hand-written searching algorithms (no std::find / std::binary_search).
class SearchEngine {
public:
    using PatientList = model::ClinicDatabase::PatientList;

    // Picks or validates a technique based on whether the data is sorted
    // by the searched field, then runs it. Throws ValidationException on bad input.
    static SearchResult search(const PatientList& patients, model::SortField sortedBy, SearchField field,
                               SearchAlgorithm algorithm, const std::string& query);

    // Linear search: exact Patient ID match, works on unsorted data. O(n)
    static int linearSearchById(const PatientList& patients, const std::string& id, long long& comparisons);
    // Linear scan for every patient whose category matches. O(n)
    static std::vector<int> linearSearchByCategory(const PatientList& patients, const std::string& keyword,
                                                   long long& comparisons);
    // Binary search on data sorted by ID. O(log n)
    static int binarySearchById(const PatientList& patients, const std::string& id, long long& comparisons);
    // Binary search for the first name starting with prefix, on data sorted by name,
    // then collects the contiguous block of matches. O(log n + k)
    static std::vector<int> binarySearchByName(const PatientList& patients, const std::string& prefix,
                                               long long& comparisons);
    // Recursive divide-and-conquer search for names containing keyword.
    // Splits the range in half each call: recursion depth O(log n), visits O(n).
    static void recursiveSearchByName(const PatientList& patients, const std::string& keyword, int low, int high,
                                      std::vector<int>& results, long long& comparisons);
    // Classic recursive binary search on ID-sorted data (used as recursive variant for IDs).
    static int recursiveBinarySearchById(const PatientList& patients, const std::string& id, int low, int high,
                                         long long& comparisons);

    static std::string fieldName(SearchField field);
};

} // namespace scapms::controller
