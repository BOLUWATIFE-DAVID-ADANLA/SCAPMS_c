# SCAPMS: Smart Clinic Appointment & Patient Management System

COS202 Programming II lab project. C++17 with Qt Widgets (Qt 6, or Qt 5.15).

## Build

**Qt Creator:** open `CMakeLists.txt` (or `SCAPMS.pro`), select a kit, press Run.

**Command line:**

```
cmake -S . -B build -DCMAKE_PREFIX_PATH=<path-to-Qt>
cmake --build build
./build/SCAPMS          # Windows: build\SCAPMS.exe (or build\Debug\SCAPMS.exe)
```

On start-up the app loads `data/clinic_data.txt` next to the executable. If it is missing,
it loads built-in sample data with dates relative to today (File > Load Sample Data does the same).

## Layout

| Folder | Responsibility |
|---|---|
| `model/` | `PatientRecord` (abstract), `GeneralPatient`, `EmergencyPatient`, `FollowUpPatient`, `Appointable` (interface), `Appointment`, `ClinicDatabase`, `PatientFactory` |
| `controller/` | `ClinicManager` (facade + undo stack), `SearchEngine`, `SortEngine`, `AppointmentController`, `ReportGenerator`, `DemoData` |
| `gui/` | `MainWindow`, `DashboardPanel`, `PatientPanel`, `AppointmentPanel`, `QueuePanel`, `SearchPanel`, `ReportsPanel`, `AdminPanel`, `BadgeDelegate` |
| `utils/` | `IDGenerator`, `FileHandler`, `Validator`, `DateUtils`, `ClinicException` hierarchy |
| `data/` | Sample data file |
| `docs/` | UML diagrams (Graphviz sources + PNG) |

Namespaces: `scapms::model`, `scapms::controller`, `scapms::gui`, `scapms::utils`.
The model and controller layers have no Qt dependency.

## Requirement map

| Requirement | Where |
|---|---|
| Abstract class with pure virtuals | `model/PatientRecord.h` |
| Interface-like class | `model/Appointable.h` |
| Runtime polymorphism | `ClinicManager::processPatient`, `triageOverview` (Dashboard) |
| Composition | `PatientRecord` owns `vector<Appointment>`; `ClinicDatabase` owns patients |
| `std::vector` | `ClinicDatabase::patients_` |
| `std::queue` | `ClinicDatabase::waitingQueue_` |
| `std::stack` (undo) | `ClinicManager::undoStack_` |
| Fixed-size array cache | `ClinicDatabase::recentPatients_[10]` |
| Linear / binary / recursive search | `controller/SearchEngine.cpp` |
| Selection / insertion / merge / quick sort | `controller/SortEngine.cpp` |
| Custom exceptions | `utils/ClinicException.h` |
| Custom table rendering | `gui/BadgeDelegate.cpp` |
| Dynamic form fields | `PatientPanel::showExtrasFor` |
| Timers | `MainWindow` (clock, dashboard refresh, reminders, long-wait alerts) |
| Persistence | `utils/FileHandler.cpp` (.txt native format, CSV import/export) |

## Shortcuts

Ctrl+N new, Ctrl+O load, Ctrl+S save, Ctrl+Z undo, Ctrl+F search, Ctrl+I / Ctrl+E import / export CSV,
Ctrl+1..7 switch tabs, Ctrl+Shift+N call next patient, Del delete patient, F5 refresh, Ctrl+Q exit.
