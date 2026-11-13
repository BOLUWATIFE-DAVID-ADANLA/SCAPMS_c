#include "SearchEngine.h"
#include "SortEngine.h"
#include "../utils/ClinicException.h"
#include "../utils/Validator.h"

#include <cctype>
#include <chrono>

namespace scapms::controller {

using model::SortField;
namespace V = utils::Validator;

std::string SearchEngine::fieldName(SearchField field) {
    switch (field) {
        case SearchField::Id: return "Patient ID";
        case SearchField::Name: return "Patient Name";
        case SearchField::Category: return "Category";
    }
    return "";
}

int SearchEngine::linearSearchById(const PatientList& patients, const std::string& id, long long& comparisons) {
    for (std::size_t i = 0; i < patients.size(); ++i) {
        ++comparisons;
        if (V::toLower(patients[i]->getId()) == V::toLower(id)) return static_cast<int>(i);
    }
    return -1;
}

std::vector<int> SearchEngine::linearSearchByCategory(const PatientList& patients, const std::string& keyword,
                                                      long long& comparisons) {
    std::vector<int> out;
    for (std::size_t i = 0; i < patients.size(); ++i) {
        ++comparisons;
        const std::string category = patients[i]->getCategoryName();
        if (V::startsWithIgnoreCase(category, keyword) || V::containsIgnoreCase(category, keyword))
            out.push_back(static_cast<int>(i));
    }
    return out;
}

// Uses the same ID ordering as the sorter, so a list sorted by ID is
// guaranteed to be ordered the way this search expects.
namespace {
int compareIdTo(const model::PatientRecord& p, const std::string& id) {
    return SortEngine::compareIds(p.getId(), id);
}
} // namespace

int SearchEngine::binarySearchById(const PatientList& patients, const std::string& id, long long& comparisons) {
    int low = 0;
    int high = static_cast<int>(patients.size()) - 1;
    while (low <= high) {
        const int mid = low + (high - low) / 2;
        ++comparisons;
        const int c = compareIdTo(*patients[mid], id);
        if (c == 0) return mid;
        if (c < 0) low = mid + 1;
        else high = mid - 1;
    }
    return -1;
}

int SearchEngine::recursiveBinarySearchById(const PatientList& patients, const std::string& id, int low, int high,
                                            long long& comparisons) {
    if (low > high) return -1;  // base case: empty range, not found
    const int mid = low + (high - low) / 2;
    ++comparisons;
    const int c = compareIdTo(*patients[mid], id);
    if (c == 0) return mid;
    if (c < 0) return recursiveBinarySearchById(patients, id, mid + 1, high, comparisons);
    return recursiveBinarySearchById(patients, id, low, mid - 1, comparisons);
}

std::vector<int> SearchEngine::binarySearchByName(const PatientList& patients, const std::string& prefix,
                                                  long long& comparisons) {
    const std::string key = V::toLower(prefix);
    // Lower bound: first index whose (lower-cased) name is >= prefix.
    int low = 0;
    int high = static_cast<int>(patients.size());
    while (low < high) {
        const int mid = low + (high - low) / 2;
        ++comparisons;
        if (V::toLower(patients[mid]->getName()) < key) low = mid + 1;
        else high = mid;
    }
    // All names sharing the prefix are contiguous from the lower bound.
    std::vector<int> out;
    for (int i = low; i < static_cast<int>(patients.size()); ++i) {
        ++comparisons;
        if (!V::startsWithIgnoreCase(patients[i]->getName(), prefix)) break;
        out.push_back(i);
    }
    return out;
}

void SearchEngine::recursiveSearchByName(const PatientList& patients, const std::string& keyword, int low, int high,
                                         std::vector<int>& results, long long& comparisons) {
    if (low > high) return;  // base case: empty range
    if (low == high) {       // base case: single record
        ++comparisons;
        if (V::containsIgnoreCase(patients[low]->getName(), keyword)) results.push_back(low);
        return;
    }
    const int mid = low + (high - low) / 2;
    recursiveSearchByName(patients, keyword, low, mid, results, comparisons);       // left half first keeps order
    recursiveSearchByName(patients, keyword, mid + 1, high, results, comparisons);
}

SearchResult SearchEngine::search(const PatientList& patients, SortField sortedBy, SearchField field,
                                  SearchAlgorithm algorithm, const std::string& rawQuery) {
    const std::string query = V::trim(rawQuery);
    if (query.empty()) throw utils::ValidationException("Please enter a search keyword.");
    if (query.size() > 60) throw utils::ValidationException("Search keyword is too long (max 60 characters).");
    if (field == SearchField::Id) {
        for (char c : query) {
            if (!std::isalnum(static_cast<unsigned char>(c)) && c != '-')
                throw utils::ValidationException("A Patient ID search may contain only letters, digits and '-'.");
        }
    }

    SearchResult result;
    const auto start = std::chrono::steady_clock::now();
    const int last = static_cast<int>(patients.size()) - 1;

    // Resolve "Auto" based on the state of the data.
    SearchAlgorithm chosen = algorithm;
    if (algorithm == SearchAlgorithm::Auto) {
        if (field == SearchField::Id) {
            chosen = sortedBy == SortField::Id ? SearchAlgorithm::Binary : SearchAlgorithm::Linear;
            result.reason = sortedBy == SortField::Id
                                ? "Records are sorted by Patient ID, so binary search halves the range each step."
                                : "Records are not sorted by Patient ID, so linear search is the only correct option.";
        } else if (field == SearchField::Name) {
            chosen = sortedBy == SortField::Name ? SearchAlgorithm::Binary : SearchAlgorithm::Recursive;
            result.reason = sortedBy == SortField::Name
                                ? "Records are sorted by name, so binary search finds the matching prefix block."
                                : "Records are not sorted by name, so a recursive divide-and-conquer scan checks "
                                  "every name for the keyword.";
        } else {
            chosen = SearchAlgorithm::Linear;
            result.reason = "Category is not a sort key, so every record must be checked (linear search).";
        }
    } else {
        result.reason = "Algorithm selected manually.";
    }

    // Binary search is only valid on data ordered by the searched field.
    if (chosen == SearchAlgorithm::Binary) {
        if (field == SearchField::Category)
            throw utils::ValidationException("Binary search is not available for Category. Use Linear or Auto.");
        const SortField needed = field == SearchField::Id ? SortField::Id : SortField::Name;
        if (sortedBy != needed)
            throw utils::ValidationException("Binary search requires the records to be sorted by " +
                                             model::sortFieldToString(needed) + " (ascending). Sort first, "
                                             "or choose Auto to let the system pick a suitable algorithm.");
    }

    switch (field) {
        case SearchField::Id:
            if (chosen == SearchAlgorithm::Binary) {
                result.algorithmUsed = "Binary Search (iterative)";
                const int idx = binarySearchById(patients, query, result.comparisons);
                if (idx >= 0) result.indices.push_back(idx);
            } else if (chosen == SearchAlgorithm::Recursive) {
                if (sortedBy == SortField::Id) {
                    result.algorithmUsed = "Recursive Binary Search";
                    const int idx = recursiveBinarySearchById(patients, query, 0, last, result.comparisons);
                    if (idx >= 0) result.indices.push_back(idx);
                } else {
                    result.algorithmUsed = "Linear Search (recursive binary search needs ID-sorted data)";
                    const int idx = linearSearchById(patients, query, result.comparisons);
                    if (idx >= 0) result.indices.push_back(idx);
                }
            } else {
                result.algorithmUsed = "Linear Search";
                const int idx = linearSearchById(patients, query, result.comparisons);
                if (idx >= 0) result.indices.push_back(idx);
            }
            break;

        case SearchField::Name:
            if (chosen == SearchAlgorithm::Binary) {
                result.algorithmUsed = "Binary Search (name prefix)";
                result.indices = binarySearchByName(patients, query, result.comparisons);
            } else if (chosen == SearchAlgorithm::Linear) {
                result.algorithmUsed = "Linear Search (name contains)";
                for (int i = 0; i <= last; ++i) {
                    ++result.comparisons;
                    if (V::containsIgnoreCase(patients[i]->getName(), query)) result.indices.push_back(i);
                }
            } else {
                result.algorithmUsed = "Recursive Search (divide and conquer)";
                recursiveSearchByName(patients, query, 0, last, result.indices, result.comparisons);
            }
            break;

        case SearchField::Category:
            result.algorithmUsed = "Linear Search";
            result.indices = linearSearchByCategory(patients, query, result.comparisons);
            break;
    }

    const auto end = std::chrono::steady_clock::now();
    result.microseconds = std::chrono::duration<double, std::micro>(end - start).count();
    return result;
}

} // namespace scapms::controller
