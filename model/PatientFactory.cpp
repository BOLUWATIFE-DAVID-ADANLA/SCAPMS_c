#include "PatientFactory.h"
#include "EmergencyPatient.h"
#include "FollowUpPatient.h"
#include "GeneralPatient.h"
#include "../utils/Validator.h"

namespace scapms::model {

std::unique_ptr<PatientRecord> PatientFactory::create(PatientCategory category, const std::string& id,
                                                      const std::string& name, int age, const std::string& contact,
                                                      const std::vector<std::string>& extras) {
    const std::string e1 = extras.size() > 0 ? extras[0] : "";
    const std::string e2 = extras.size() > 1 ? extras[1] : "";

    switch (category) {
        case PatientCategory::General:
            return std::make_unique<GeneralPatient>(id, name, age, contact, e1, e2);
        case PatientCategory::Emergency: {
            const int severity = e1.empty() ? 3 : utils::Validator::parseSeverity(e1);
            return std::make_unique<EmergencyPatient>(id, name, age, contact, severity, e2);
        }
        case PatientCategory::FollowUp:
            return std::make_unique<FollowUpPatient>(id, name, age, contact, e1, e2);
    }
    return nullptr;
}

} // namespace scapms::model
