#pragma once

#include "../model/ClinicDatabase.h"

namespace scapms::controller {

// Seeds a realistic sample data set with dates relative to today, so
// reminders, daily summaries and reports always have something to show.
class DemoData {
public:
    static void populate(model::ClinicDatabase& db);
};

} // namespace scapms::controller
