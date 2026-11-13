# qmake project (alternative to CMakeLists.txt) for Qt Creator users
QT       += core gui widgets
CONFIG   += c++17
TARGET    = SCAPMS
TEMPLATE  = app

SOURCES += main.cpp \
    model/Appointment.cpp model/PatientRecord.cpp model/GeneralPatient.cpp \
    model/EmergencyPatient.cpp model/FollowUpPatient.cpp model/PatientFactory.cpp model/ClinicDatabase.cpp \
    controller/ClinicManager.cpp controller/SearchEngine.cpp controller/SortEngine.cpp \
    controller/AppointmentController.cpp controller/ReportGenerator.cpp controller/DemoData.cpp \
    utils/IDGenerator.cpp utils/FileHandler.cpp utils/Validator.cpp utils/DateUtils.cpp \
    gui/BadgeDelegate.cpp gui/MainWindow.cpp gui/DashboardPanel.cpp gui/PatientPanel.cpp \
    gui/AppointmentPanel.cpp gui/QueuePanel.cpp gui/SearchPanel.cpp gui/ReportsPanel.cpp gui/AdminPanel.cpp

HEADERS += \
    model/Appointable.h model/Appointment.h model/PatientRecord.h model/GeneralPatient.h \
    model/EmergencyPatient.h model/FollowUpPatient.h model/PatientFactory.h model/ClinicDatabase.h \
    controller/ClinicManager.h controller/SearchEngine.h controller/SortEngine.h \
    controller/AppointmentController.h controller/ReportGenerator.h controller/DemoData.h \
    utils/ClinicException.h utils/IDGenerator.h utils/FileHandler.h utils/Validator.h utils/DateUtils.h \
    gui/GuiHelpers.h gui/BasePanel.h gui/BadgeDelegate.h gui/MainWindow.h gui/DashboardPanel.h \
    gui/PatientPanel.h gui/AppointmentPanel.h gui/QueuePanel.h gui/SearchPanel.h gui/ReportsPanel.h gui/AdminPanel.h

# Copy sample data next to the executable
datafiles.files = data/clinic_data.txt
datafiles.path  = $$OUT_PWD/data
COPIES += datafiles
