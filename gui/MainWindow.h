#pragma once

#include "../controller/ClinicManager.h"

#include <QMainWindow>
#include <QSet>

class QAction;
class QLabel;
class QTabWidget;
class QTimer;

namespace scapms::gui {

class AdminPanel;
class AppointmentPanel;
class BasePanel;
class DashboardPanel;
class PatientPanel;
class QueuePanel;
class ReportsPanel;
class SearchPanel;

// Top-level window: owns the ClinicManager, builds menus/toolbar/status bar,
// hosts the tab panels and drives the timers (clock, dashboard refresh,
// appointment reminders, long-wait alerts).
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

    // Opens the default data file, or loads sample data if none exists.
    void loadStartupData();
    QTabWidget* tabs() const { return tabs_; }

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void newSession();
    void openFile();
    bool saveFile();
    bool saveFileAs();
    void importCsv();
    void exportCsv();
    void loadSampleData();
    void undo();
    void focusSearch();
    void refreshAll();
    void showStatus(const QString& message, int timeoutMs = 4000);
    void onClockTick();
    void onDashboardTimer();
    void onReminderTimer();
    void showShortcuts();
    void showAbout();

private:
    void createActions();
    void createMenus();
    void createToolBar();
    void createStatusBar();
    void connectPanel(BasePanel* panel);
    bool maybeSave();
    void updateTitle();
    QString defaultDataPath() const;

    controller::ClinicManager manager_;

    QTabWidget* tabs_;
    DashboardPanel* dashboard_;
    PatientPanel* patients_;
    AppointmentPanel* appointments_;
    QueuePanel* queue_;
    SearchPanel* search_;
    ReportsPanel* reports_;
    AdminPanel* admin_;

    QAction* newAct_;
    QAction* openAct_;
    QAction* saveAct_;
    QAction* saveAsAct_;
    QAction* importAct_;
    QAction* exportAct_;
    QAction* sampleAct_;
    QAction* exitAct_;
    QAction* undoAct_;
    QAction* findAct_;
    QAction* refreshAct_;

    QLabel* sortLabel_;
    QLabel* countLabel_;
    QLabel* fileLabel_;
    QLabel* clockLabel_;

    QTimer* clockTimer_;      // 1 s: status bar clock
    QTimer* dashboardTimer_;  // 30 s: refresh dashboard + queue waiting times
    QTimer* reminderTimer_;   // 30 s: appointment reminders and long-wait alerts

    QSet<QString> remindedAppointments_;
    QSet<QString> alertedWaits_;
};

} // namespace scapms::gui
