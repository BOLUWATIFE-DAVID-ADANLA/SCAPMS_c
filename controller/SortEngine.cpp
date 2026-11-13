#include "SortEngine.h"
#include "../utils/Validator.h"

#include <cctype>
#include <chrono>
#include <utility>

namespace scapms::controller {

using model::PatientRecord;
using model::SortField;

namespace {
int compareStrings(const std::string& a, const std::string& b) {
    const std::string la = utils::Validator::toLower(a);
    const std::string lb = utils::Validator::toLower(b);
    if (la < lb) return -1;
    if (la > lb) return 1;
    return 0;
}

} // namespace

int SortEngine::compareIds(const std::string& a, const std::string& b) {
    std::size_t i = 0, j = 0;
    while (i < a.size() && j < b.size()) {
        const bool da = std::isdigit(static_cast<unsigned char>(a[i])) != 0;
        const bool db = std::isdigit(static_cast<unsigned char>(b[j])) != 0;
        if (da && db) {
            std::size_t ei = i, ej = j;
            while (ei < a.size() && std::isdigit(static_cast<unsigned char>(a[ei]))) ++ei;
            while (ej < b.size() && std::isdigit(static_cast<unsigned char>(b[ej]))) ++ej;
            std::string na = a.substr(i, ei - i), nb = b.substr(j, ej - j);
            while (na.size() > 1 && na[0] == '0') na.erase(0, 1);
            while (nb.size() > 1 && nb[0] == '0') nb.erase(0, 1);
            if (na.size() != nb.size()) return na.size() < nb.size() ? -1 : 1;
            if (na != nb) return na < nb ? -1 : 1;
            i = ei;
            j = ej;
        } else {
            const char ca = static_cast<char>(std::tolower(static_cast<unsigned char>(a[i])));
            const char cb = static_cast<char>(std::tolower(static_cast<unsigned char>(b[j])));
            if (ca != cb) return ca < cb ? -1 : 1;
            ++i;
            ++j;
        }
    }
    if (i < a.size()) return 1;
    if (j < b.size()) return -1;
    return 0;
}

int SortEngine::compare(const PatientRecord& a, const PatientRecord& b, SortField field) {
    int result = 0;
    switch (field) {
        case SortField::Name:
            result = compareStrings(a.getName(), b.getName());
            break;
        case SortField::Id:
            return compareIds(a.getId(), b.getId());  // IDs are unique, no tie-break needed
        case SortField::Age:
            result = a.getAge() - b.getAge();
            break;
        case SortField::AppointmentDate: {
            // Patients with no pending appointment go to the end.
            std::string da = a.nextAppointmentDateTime(), db = b.nextAppointmentDateTime();
            if (da.empty()) da = "9999-12-31 23:59";
            if (db.empty()) db = "9999-12-31 23:59";
            result = da.compare(db);
            break;
        }
        case SortField::None:
            break;
    }
    if (result != 0) return result < 0 ? -1 : 1;
    return compareIds(a.getId(), b.getId());  // deterministic tie-break
}

int SortEngine::Context::cmp(const PatientRecord& a, const PatientRecord& b) {
    ++stats.comparisons;
    const int r = SortEngine::compare(a, b, field);
    return ascending ? r : -r;
}

SortStats SortEngine::sort(PatientList& patients, SortAlgorithm algorithm, SortField field, bool ascending) {
    Context ctx{field, ascending, {}};
    const auto start = std::chrono::steady_clock::now();

    if (patients.size() > 1) {
        switch (algorithm) {
            case SortAlgorithm::Selection: selectionSort(patients, ctx); break;
            case SortAlgorithm::Insertion: insertionSort(patients, ctx); break;
            case SortAlgorithm::Merge: {
                PatientList buffer(patients.size());
                mergeSort(patients, buffer, 0, static_cast<int>(patients.size()) - 1, ctx);
                break;
            }
            case SortAlgorithm::Quick:
                quickSort(patients, 0, static_cast<int>(patients.size()) - 1, ctx);
                break;
        }
    }

    const auto end = std::chrono::steady_clock::now();
    ctx.stats.microseconds = std::chrono::duration<double, std::micro>(end - start).count();
    return ctx.stats;
}

// Selection sort: repeatedly select the smallest remaining element and
// swap it into place. O(n^2) comparisons but at most n-1 swaps.
void SortEngine::selectionSort(PatientList& v, Context& ctx) {
    const int n = static_cast<int>(v.size());
    for (int i = 0; i < n - 1; ++i) {
        int minIndex = i;
        for (int j = i + 1; j < n; ++j) {
            if (ctx.cmp(*v[j], *v[minIndex]) < 0) minIndex = j;
        }
        if (minIndex != i) {
            std::swap(v[i], v[minIndex]);
            ++ctx.stats.moves;
        }
    }
}

// Insertion sort: grow a sorted prefix by inserting each new element into
// its correct position. Stable, and O(n) when the data is already sorted.
void SortEngine::insertionSort(PatientList& v, Context& ctx) {
    const int n = static_cast<int>(v.size());
    for (int i = 1; i < n; ++i) {
        std::unique_ptr<PatientRecord> key = std::move(v[i]);
        int j = i - 1;
        while (j >= 0 && ctx.cmp(*v[j], *key) > 0) {
            v[j + 1] = std::move(v[j]);
            ++ctx.stats.moves;
            --j;
        }
        v[j + 1] = std::move(key);
        ++ctx.stats.moves;
    }
}

// Merge sort (recursive divide and conquer): split, sort each half, merge.
// Stable and always O(n log n), at the cost of an O(n) buffer.
void SortEngine::mergeSort(PatientList& v, PatientList& buffer, int left, int right, Context& ctx) {
    if (left >= right) return;  // base case: 0 or 1 element is sorted
    const int mid = left + (right - left) / 2;
    mergeSort(v, buffer, left, mid, ctx);
    mergeSort(v, buffer, mid + 1, right, ctx);
    merge(v, buffer, left, mid, right, ctx);
}

void SortEngine::merge(PatientList& v, PatientList& buffer, int left, int mid, int right, Context& ctx) {
    int i = left, j = mid + 1, k = left;
    while (i <= mid && j <= right) {
        // "<= 0" keeps equal elements in their original order (stability).
        if (ctx.cmp(*v[i], *v[j]) <= 0) buffer[k++] = std::move(v[i++]);
        else buffer[k++] = std::move(v[j++]);
        ++ctx.stats.moves;
    }
    while (i <= mid) { buffer[k++] = std::move(v[i++]); ++ctx.stats.moves; }
    while (j <= right) { buffer[k++] = std::move(v[j++]); ++ctx.stats.moves; }
    for (int x = left; x <= right; ++x) v[x] = std::move(buffer[x]);
}

// Quick sort with median-of-three pivot selection (avoids the O(n^2)
// worst case on already-sorted input). Average O(n log n), in place.
void SortEngine::quickSort(PatientList& v, int low, int high, Context& ctx) {
    while (low < high) {
        const int p = partition(v, low, high, ctx);
        // Recurse into the smaller side, loop on the larger: keeps stack depth O(log n).
        if (p - low < high - p) {
            quickSort(v, low, p - 1, ctx);
            low = p + 1;
        } else {
            quickSort(v, p + 1, high, ctx);
            high = p - 1;
        }
    }
}

int SortEngine::partition(PatientList& v, int low, int high, Context& ctx) {
    const int mid = low + (high - low) / 2;
    if (ctx.cmp(*v[mid], *v[low]) < 0) { std::swap(v[mid], v[low]); ++ctx.stats.moves; }
    if (ctx.cmp(*v[high], *v[low]) < 0) { std::swap(v[high], v[low]); ++ctx.stats.moves; }
    if (ctx.cmp(*v[mid], *v[high]) < 0) { std::swap(v[mid], v[high]); ++ctx.stats.moves; }
    // Median of the three is now at v[high]; use it as the pivot (Lomuto scheme).
    PatientRecord& pivot = *v[high];
    int i = low - 1;
    for (int j = low; j < high; ++j) {
        if (ctx.cmp(*v[j], pivot) < 0) {
            ++i;
            if (i != j) { std::swap(v[i], v[j]); ++ctx.stats.moves; }
        }
    }
    if (i + 1 != high) { std::swap(v[i + 1], v[high]); ++ctx.stats.moves; }
    return i + 1;
}

std::string SortEngine::algorithmName(SortAlgorithm algorithm) {
    switch (algorithm) {
        case SortAlgorithm::Selection: return "Selection Sort";
        case SortAlgorithm::Insertion: return "Insertion Sort";
        case SortAlgorithm::Merge: return "Merge Sort";
        case SortAlgorithm::Quick: return "Quick Sort";
    }
    return "";
}

std::string SortEngine::complexity(SortAlgorithm algorithm) {
    switch (algorithm) {
        case SortAlgorithm::Selection: return "O(n^2) time, O(1) space, not stable";
        case SortAlgorithm::Insertion: return "O(n^2) worst / O(n) best time, O(1) space, stable";
        case SortAlgorithm::Merge: return "O(n log n) time, O(n) space, stable";
        case SortAlgorithm::Quick: return "O(n log n) average / O(n^2) worst time, O(log n) space, not stable";
    }
    return "";
}

SortAlgorithm SortEngine::suggest(std::size_t n, bool nearlySorted) {
    if (nearlySorted || n <= 20) return SortAlgorithm::Insertion;
    if (n <= 1000) return SortAlgorithm::Quick;
    return SortAlgorithm::Merge;
}

std::string SortEngine::recommendation(SortAlgorithm algorithm, std::size_t n, bool alreadySortedBySameField) {
    const std::string size = std::to_string(n) + " records";
    switch (algorithm) {
        case SortAlgorithm::Selection:
            return "Selection Sort is suitable here because the clinic list is small (" + size +
                   ") and it performs at most n-1 swaps, so it moves records as little as possible. "
                   "For large lists its O(n^2) comparisons make it slow.";
        case SortAlgorithm::Insertion:
            return std::string("Insertion Sort is suitable for small or nearly sorted lists (") + size +
                   (alreadySortedBySameField ? ", already in order" : "") +
                   "). It is stable, sorts in place, and runs in linear time when new patients have only "
                   "been appended to an ordered list.";
        case SortAlgorithm::Merge:
            return "Merge Sort guarantees O(n log n) regardless of the input order and is stable, so patients "
                   "with equal keys (e.g. same age) keep their previous relative order. Best choice as the "
                   "patient list grows (" + size + ").";
        case SortAlgorithm::Quick:
            return "Quick Sort is usually the fastest in practice for medium/large unsorted lists (" + size +
                   ") and sorts in place. Median-of-three pivoting avoids the worst case on already sorted data.";
    }
    return "";
}

} // namespace scapms::controller
