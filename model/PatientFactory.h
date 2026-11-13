#pragma once

#include "PatientRecord.h"

#include <memory>
#include <string>
#include <vector>

namespace scapms::model {

// Creates the right PatientRecord subclass from a category and raw field values.
// extras holds the two type-specific fields in the order of getExtraFieldLabels().
class PatientFactory {
public:
    static std::unique_ptr<PatientRecord> create(PatientCategory category, const std::string& id,
                                                 const std::string& name, int age, const std::string& contact,
                                                 const std::vector<std::string>& extras);
};

} // namespace scapms::model
