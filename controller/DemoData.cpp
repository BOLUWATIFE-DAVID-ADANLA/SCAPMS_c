#include "DemoData.h"
#include "../model/PatientFactory.h"
#include "../utils/DateUtils.h"

#include <cstdio>
#include <string>

namespace scapms::controller {

using model::Appointment;
using model::AppointmentStatus;
using model::PatientCategory;
using utils::DateUtils::addDays;

namespace {
struct Seed {
    PatientCategory category;
    const char* name;
    int age;
    const char* contact;
    const char* extra1;
    const char* extra2;
};
} // namespace

void DemoData::populate(model::ClinicDatabase& db) {
    const std::string today = utils::DateUtils::today();

    const Seed seeds[] = {
        {PatientCategory::General, "Adaeze Okafor", 21, "08031234567", "General Medicine", "NHIS-44120"},
        {PatientCategory::Emergency, "Tunde Bakare", 34, "08029876543", "4", "Severe asthma attack"},
        {PatientCategory::FollowUp, "Chiamaka Eze", 19, "chiamaka.eze@student.edu.ng", "", "Dr. Adeyemi"},
        {PatientCategory::General, "Ibrahim Musa", 45, "07061122334", "Dental", ""},
        {PatientCategory::General, "Funmilayo Adebayo", 52, "08134455667", "Eye Clinic", "NHIS-90311"},
        {PatientCategory::Emergency, "Emeka Nwosu", 27, "09015566778", "5", "Road traffic injury"},
        {PatientCategory::FollowUp, "Zainab Abdullahi", 23, "08167788990", "", "Dr. Okonkwo"},
        {PatientCategory::General, "Oluwaseun Ajayi", 20, "seun.ajayi@student.edu.ng", "General Medicine", ""},
        {PatientCategory::FollowUp, "Ngozi Obi", 61, "08051239876", "", "Dr. Adeyemi"},
        {PatientCategory::General, "Kelechi Uche", 8, "08099988877", "Paediatrics", "NHIS-11873"},
        {PatientCategory::Emergency, "Halima Sani", 40, "07034561234", "3", "High fever and dehydration"},
        {PatientCategory::General, "Bolaji Ogunleye", 30, "08187654321", "Physiotherapy", ""},
        {PatientCategory::FollowUp, "Yetunde Lawal", 35, "yetunde.lawal@unimail.ng", "", "Dr. Bello"},
        {PatientCategory::General, "Daniel Etim", 72, "08023344556", "General Medicine", "NHIS-77001"},
        {PatientCategory::General, "Amina Garba", 16, "08145566778", "General Medicine", ""},
        {PatientCategory::FollowUp, "Victor Okoro", 47, "09076655443", "", "Dr. Okonkwo"},
        {PatientCategory::Emergency, "Grace Johnson", 25, "08112233445", "2", "Allergic reaction"},
        {PatientCategory::General, "Musa Danjuma", 38, "07087766554", "Dental", ""},
    };

    // Inserted deliberately out of ID order so sorting has visible work to do.
    const int order[] = {5, 0, 12, 3, 17, 8, 1, 14, 6, 10, 2, 15, 9, 4, 13, 7, 16, 11};
    for (int idx : order) {
        const Seed& s = seeds[idx];
        std::string extra1 = s.extra1;
        if (s.category == PatientCategory::FollowUp) extra1 = addDays(today, -(14 + idx));  // previous visit
        char id[16];
        std::snprintf(id, sizeof(id), "P%04d", idx + 1);
        db.addPatient(model::PatientFactory::create(s.category, id, s.name, s.age, s.contact, {extra1, s.extra2}));
    }

    int apptCounter = 0;
    auto add = [&](int patientNo, int dayOffset, const char* time, const char* reason, AppointmentStatus status) {
        char pid[16], aid[16];
        std::snprintf(pid, sizeof(pid), "P%04d", patientNo);
        std::snprintf(aid, sizeof(aid), "A%04d", ++apptCounter);
        db.getPatient(pid)->attachAppointment(Appointment(aid, pid, addDays(today, dayOffset), time, reason, status));
    };

    using S = AppointmentStatus;
    // History (completed / cancelled) - drives "most frequently attended".
    add(1, -60, "09:00", "Routine check-up", S::Completed);
    add(1, -30, "10:30", "Malaria treatment review", S::Completed);
    add(1, -7, "11:00", "Blood test results", S::Completed);
    add(3, -14, "09:30", "Initial consultation", S::Completed);
    add(3, -5, "14:00", "Medication review", S::Completed);
    add(4, -20, "13:00", "Tooth extraction", S::Completed);
    add(5, -45, "10:00", "Eye examination", S::Completed);
    add(5, -15, "10:00", "Glasses fitting", S::Completed);
    add(5, -3, "15:00", "Pressure check", S::Cancelled);
    add(9, -50, "08:30", "Hypertension diagnosis", S::Completed);
    add(9, -40, "08:30", "Hypertension review", S::Completed);
    add(9, -20, "08:30", "Hypertension review", S::Completed);
    add(9, -10, "08:30", "Hypertension review", S::Completed);
    add(9, -2, "08:30", "Hypertension review", S::Completed);
    add(10, -12, "11:30", "Immunisation", S::Completed);
    add(12, -9, "16:00", "Back pain therapy", S::Completed);
    add(12, -2, "16:00", "Back pain therapy", S::Completed);
    add(14, -25, "09:00", "Diabetes check", S::Completed);
    add(14, -4, "09:00", "Diabetes check", S::Cancelled);
    add(2, -1, "12:00", "Asthma emergency", S::Completed);

    // Today - feeds the daily summary, dashboard and reminder timer.
    add(1, 0, "09:00", "Follow-up blood test", S::Completed);
    add(4, 0, "10:00", "Dental filling", S::Completed);
    add(7, 0, "11:30", "Post-treatment review", S::Pending);
    add(8, 0, "13:00", "Sports injury check", S::Pending);
    add(11, 0, "14:30", "Fever monitoring", S::Pending);
    add(15, 0, "15:00", "Sick note request", S::Cancelled);
    add(18, 0, "16:30", "Toothache", S::Pending);

    // Upcoming.
    add(3, 2, "10:00", "Lab results discussion", S::Pending);
    add(6, 1, "09:00", "Wound dressing", S::Pending);
    add(9, 7, "08:30", "Hypertension review", S::Pending);
    add(13, 3, "11:00", "Post-surgery review", S::Pending);
    add(16, 5, "14:00", "Physio progress check", S::Pending);
    add(10, 14, "10:30", "Booster vaccination", S::Pending);
    add(17, 1, "12:30", "Allergy test", S::Pending);

    // Waiting queue (FIFO), joined a few minutes apart; the first has waited over 30 minutes.
    const long long now = utils::DateUtils::nowEpoch();
    db.enqueue("P0002", now - 42 * 60);
    db.enqueue("P0008", now - 25 * 60);
    db.enqueue("P0015", now - 12 * 60);
    db.enqueue("P0011", now - 4 * 60);

    db.setSortedBy(model::SortField::None);
}

} // namespace scapms::controller
