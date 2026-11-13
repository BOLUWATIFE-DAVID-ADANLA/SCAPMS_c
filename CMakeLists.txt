cmake_minimum_required(VERSION 3.16)
project(SCAPMS VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# Works with Qt 6 (preferred) or Qt 5.15
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

set(MODEL_SOURCES
    model/Appointable.h
    model/Appointment.h          model/Appointment.cpp
    model/PatientRecord.h        model/PatientRecord.cpp
    model/GeneralPatient.h       model/GeneralPatient.cpp
    model/EmergencyPatient.h     model/EmergencyPatient.cpp
    model/FollowUpPatient.h      model/FollowUpPatient.cpp
    model/PatientFactory.h       model/PatientFactory.cpp
    model/ClinicDatabase.h       model/ClinicDatabase.cpp
)

set(CONTROLLER_SOURCES
    controller/ClinicManager.h         controller/ClinicManager.cpp
    controller/SearchEngine.h          controller/SearchEngine.cpp
    controller/SortEngine.h            controller/SortEngine.cpp
    controller/AppointmentController.h controller/AppointmentController.cpp
    controller/ReportGenerator.h       controller/ReportGenerator.cpp
    controller/DemoData.h              controller/DemoData.cpp
)

set(UTILS_SOURCES
    utils/ClinicException.h
    utils/IDGenerator.h  utils/IDGenerator.cpp
    utils/FileHandler.h  utils/FileHandler.cpp
    utils/Validator.h    utils/Validator.cpp
    utils/DateUtils.h    utils/DateUtils.cpp
)

set(GUI_SOURCES
    gui/GuiHelpers.h
    gui/BasePanel.h
    gui/BadgeDelegate.h     gui/BadgeDelegate.cpp
    gui/MainWindow.h        gui/MainWindow.cpp
    gui/DashboardPanel.h    gui/DashboardPanel.cpp
    gui/PatientPanel.h      gui/PatientPanel.cpp
    gui/AppointmentPanel.h  gui/AppointmentPanel.cpp
    gui/QueuePanel.h        gui/QueuePanel.cpp
    gui/SearchPanel.h       gui/SearchPanel.cpp
    gui/ReportsPanel.h      gui/ReportsPanel.cpp
    gui/AdminPanel.h        gui/AdminPanel.cpp
)

add_executable(SCAPMS main.cpp ${MODEL_SOURCES} ${CONTROLLER_SOURCES} ${UTILS_SOURCES} ${GUI_SOURCES})
target_link_libraries(SCAPMS PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

if(MSVC)
    target_compile_options(SCAPMS PRIVATE /W4 /utf-8)
else()
    target_compile_options(SCAPMS PRIVATE -Wall -Wextra)
endif()

set_target_properties(SCAPMS PROPERTIES WIN32_EXECUTABLE ON MACOSX_BUNDLE OFF)

# Copy the sample data file next to the executable on first build only,
# so a rebuild never overwrites data saved while using the app.
add_custom_command(TARGET SCAPMS POST_BUILD
    COMMAND ${CMAKE_COMMAND}
            -DSRC=${CMAKE_SOURCE_DIR}/data/clinic_data.txt
            -DDST=$<TARGET_FILE_DIR:SCAPMS>/data/clinic_data.txt
            -P ${CMAKE_SOURCE_DIR}/cmake/CopyIfMissing.cmake)
