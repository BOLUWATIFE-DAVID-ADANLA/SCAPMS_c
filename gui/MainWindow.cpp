#include "MainWindow.h"
#include "AdminPanel.h"
#include "AppointmentPanel.h"
#include "DashboardPanel.h"
#include "GuiHelpers.h"
#include "PatientPanel.h"
#include "QueuePanel.h"
#include "ReportsPanel.h"
#include "SearchPanel.h"
#include "../utils/DateUtils.h"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <QStyle>
#include <QTabWidget>
#include <QTimer>
#include <QToolBar>

namespace scapms::gui {

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("SCAPMS");
    setMinimumSize(1100, 700);

    tabs_ = new QTabWidget;
    tabs_->setDocumentMode(true);
    dashboard_ = new DashboardPanel(manager_);
    patients_ = new PatientPanel(manager_);
    appointments_ = new AppointmentPanel(manager_);
    queue_ = new QueuePanel(manager_);
    search_ = new SearchPanel(manager_);
    reports_ = new ReportsPanel(manager_);
    admin_ = new AdminPanel(manager_);

    tabs_->addTab(dashboard_, "Dashboard");
    tabs_->addTab(patients_, "Patient Records");
    tabs_->addTab(appointments_, "Appointments");
    tabs_->addTab(queue_, "Waiting Queue");
    tabs_->addTab(search_, "Search && Sort");
    tabs_->addTab(reports_, "Reports");
    tabs_->addTab(admin_, "Admin");
    tabs_->setTabToolTip(0, "Overview statistics (Ctrl+1)");
    tabs_->setTabToolTip(1, "Register, edit and delete patients (Ctrl+2)");
    tabs_->setTabToolTip(2, "Schedule and manage appointments (Ctrl+3)");
    tabs_->setTabToolTip(3, "First-come, first-served consultation queue (Ctrl+4)");
    tabs_->setTabToolTip(4, "Search and sort patient records (Ctrl+5)");
    tabs_->setTabToolTip(5, "Clinic reports (Ctrl+6)");
    tabs_->setTabToolTip(6, "Undo history, cache and data management (Ctrl+7)");
    setCentralWidget(tabs_);

    for (BasePanel* p : std::initializer_list<BasePanel*>{dashboard_, patients_, appointments_, queue_, search_,
                                                          reports_, admin_})
        connectPanel(p);
    connect(admin_, &AdminPanel::undoRequested, this, &MainWindow::undo);
    connect(admin_, &AdminPanel::newSessionRequested, this, &MainWindow::newSession);
    connect(admin_, &AdminPanel::sampleDataRequested, this, &MainWindow::loadSampleData);
    connect(tabs_, &QTabWidget::currentChanged, this, [this](int index) {
        if (index == 3) tabs_->setTabIcon(3, QIcon());  // clear the long-wait warning once viewed
    });

    createActions();
    createMenus();
    createToolBar();
    createStatusBar();

    // Timer events
    clockTimer_ = new QTimer(this);
    connect(clockTimer_, &QTimer::timeout, this, &MainWindow::onClockTick);
    clockTimer_->start(1000);

    dashboardTimer_ = new QTimer(this);
    connect(dashboardTimer_, &QTimer::timeout, this, &MainWindow::onDashboardTimer);
    dashboardTimer_->start(30 * 1000);

    reminderTimer_ = new QTimer(this);
    connect(reminderTimer_, &QTimer::timeout, this, &MainWindow::onReminderTimer);
    reminderTimer_->start(30 * 1000);

    onClockTick();
}

void MainWindow::connectPanel(BasePanel* panel) {
    connect(panel, &BasePanel::dataChanged, this, &MainWindow::refreshAll);
    connect(panel, &BasePanel::statusMessage, this, &MainWindow::showStatus);
}

// ===================================================================== actions & menus

void MainWindow::createActions() {
    QStyle* st = style();
    newAct_ = new QAction(st->standardIcon(QStyle::SP_FileIcon), "&New", this);
    newAct_->setShortcut(QKeySequence::New);
    newAct_->setToolTip("Start a new, empty clinic session (Ctrl+N)");
    connect(newAct_, &QAction::triggered, this, &MainWindow::newSession);

    openAct_ = new QAction(st->standardIcon(QStyle::SP_DialogOpenButton), "&Load...", this);
    openAct_->setShortcut(QKeySequence::Open);
    openAct_->setToolTip("Load patient and appointment data from a file (Ctrl+O)");
    connect(openAct_, &QAction::triggered, this, &MainWindow::openFile);

    saveAct_ = new QAction(st->standardIcon(QStyle::SP_DialogSaveButton), "&Save", this);
    saveAct_->setShortcut(QKeySequence::Save);
    saveAct_->setToolTip("Save all data (Ctrl+S)");
    connect(saveAct_, &QAction::triggered, this, &MainWindow::saveFile);

    saveAsAct_ = new QAction("Save &As...", this);
    saveAsAct_->setShortcut(QKeySequence("Ctrl+Shift+S"));
    connect(saveAsAct_, &QAction::triggered, this, &MainWindow::saveFileAs);

    importAct_ = new QAction("&Import Patients (CSV)...", this);
    importAct_->setShortcut(QKeySequence("Ctrl+I"));
    importAct_->setToolTip("Add patients from a CSV file (Ctrl+I)");
    connect(importAct_, &QAction::triggered, this, &MainWindow::importCsv);

    exportAct_ = new QAction("&Export Patients (CSV)...", this);
    exportAct_->setShortcut(QKeySequence("Ctrl+E"));
    exportAct_->setToolTip("Export patient details to a CSV file (Ctrl+E)");
    connect(exportAct_, &QAction::triggered, this, &MainWindow::exportCsv);

    sampleAct_ = new QAction("Load Sample &Data", this);
    connect(sampleAct_, &QAction::triggered, this, &MainWindow::loadSampleData);

    exitAct_ = new QAction("E&xit", this);
    exitAct_->setShortcut(QKeySequence("Ctrl+Q"));
    connect(exitAct_, &QAction::triggered, this, &QWidget::close);

    undoAct_ = new QAction(st->standardIcon(QStyle::SP_ArrowBack), "&Undo Last Operation", this);
    undoAct_->setShortcut(QKeySequence::Undo);
    undoAct_->setToolTip("Undo the last administrative operation (Ctrl+Z)");
    connect(undoAct_, &QAction::triggered, this, &MainWindow::undo);

    findAct_ = new QAction(st->standardIcon(QStyle::SP_FileDialogContentsView), "&Find Patient", this);
    findAct_->setShortcut(QKeySequence("Ctrl+F"));
    findAct_->setToolTip("Go to Search & Sort (Ctrl+F)");
    connect(findAct_, &QAction::triggered, this, &MainWindow::focusSearch);

    refreshAct_ = new QAction(st->standardIcon(QStyle::SP_BrowserReload), "&Refresh", this);
    refreshAct_->setShortcut(QKeySequence::Refresh);
    refreshAct_->setToolTip("Refresh all panels (F5)");
    connect(refreshAct_, &QAction::triggered, this, &MainWindow::refreshAll);

    // Ctrl+1..7 switch tabs
    for (int i = 0; i < tabs_->count(); ++i) {
        auto* act = new QAction(this);
        act->setShortcut(QKeySequence(QString("Ctrl+%1").arg(i + 1)));
        connect(act, &QAction::triggered, this, [this, i] { tabs_->setCurrentIndex(i); });
        addAction(act);
    }
}

void MainWindow::createMenus() {
    QMenu* file = menuBar()->addMenu("&File");
    file->addAction(newAct_);
    file->addAction(openAct_);
    file->addAction(saveAct_);
    file->addAction(saveAsAct_);
    file->addSeparator();
    file->addAction(importAct_);
    file->addAction(exportAct_);
    file->addAction(sampleAct_);
    file->addSeparator();
    file->addAction(exitAct_);

    QMenu* edit = menuBar()->addMenu("&Edit");
    edit->addAction(undoAct_);
    edit->addAction(findAct_);

    QMenu* view = menuBar()->addMenu("&View");
    view->addAction(refreshAct_);
    view->addSeparator();
    for (int i = 0; i < tabs_->count(); ++i) {
        QAction* a = view->addAction(tabs_->tabText(i).replace("&&", "&"));
        a->setShortcut(QKeySequence(QString("Ctrl+%1").arg(i + 1)));
        a->setShortcutContext(Qt::WidgetShortcut);  // shown in menu; window-level actions do the work
        connect(a, &QAction::triggered, this, [this, i] { tabs_->setCurrentIndex(i); });
    }

    QMenu* help = menuBar()->addMenu("&Help");
    help->addAction("&Keyboard Shortcuts", this, &MainWindow::showShortcuts);
    help->addAction("&About SCAPMS", this, &MainWindow::showAbout);
}

void MainWindow::createToolBar() {
    QToolBar* tb = addToolBar("Main");
    tb->setMovable(false);
    tb->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    tb->addAction(newAct_);
    tb->addAction(openAct_);
    tb->addAction(saveAct_);
    tb->addSeparator();
    tb->addAction(undoAct_);
    tb->addAction(findAct_);
    tb->addAction(refreshAct_);
}

void MainWindow::createStatusBar() {
    sortLabel_ = new QLabel;
    countLabel_ = new QLabel;
    fileLabel_ = new QLabel;
    clockLabel_ = new QLabel;
    for (QLabel* l : {sortLabel_, countLabel_, fileLabel_, clockLabel_}) {
        l->setObjectName("statusPill");
        statusBar()->addPermanentWidget(l);
    }
    sortLabel_->setToolTip("Field the patient list is currently sorted by");
    fileLabel_->setToolTip("Current data file");
    statusBar()->showMessage("Ready");
}

// ===================================================================== refresh & status

void MainWindow::refreshAll() {
    dashboard_->refresh();
    patients_->refresh();
    appointments_->refresh();
    queue_->refresh();
    search_->refresh();
    reports_->refresh();
    admin_->refresh();

    const auto& db = manager_.database();
    sortLabel_->setText("Order: " + q(model::sortFieldToString(db.sortedBy())));
    countLabel_->setText(QString("%1 patients | %2 waiting").arg(db.size()).arg(db.queueSize()));
    fileLabel_->setText(manager_.currentFile().empty() ? "Unsaved session"
                                                       : QFileInfo(q(manager_.currentFile())).fileName());
    undoAct_->setEnabled(manager_.canUndo());
    undoAct_->setText(manager_.canUndo() ? "&Undo: " + q(manager_.undoHistory().front()) : "&Undo Last Operation");
    updateTitle();
}

void MainWindow::updateTitle() {
    const QString name = manager_.currentFile().empty() ? "Untitled" : QFileInfo(q(manager_.currentFile())).fileName();
    setWindowTitle(QString("SCAPMS - Smart Clinic Appointment & Patient Management - %1[*]").arg(name));
    setWindowModified(manager_.isDirty());
}

void MainWindow::showStatus(const QString& message, int timeoutMs) { statusBar()->showMessage(message, timeoutMs); }

// ===================================================================== timers

void MainWindow::onClockTick() {
    clockLabel_->setText(QDateTime::currentDateTime().toString("ddd dd MMM yyyy  HH:mm:ss"));
}

void MainWindow::onDashboardTimer() {
    dashboard_->refresh();
    queue_->refreshWaitingTimes();
}

void MainWindow::onReminderTimer() {
    // 1) Appointment reminders: pending appointments starting within 15 minutes.
    const auto soon = manager_.appointments().upcoming(utils::DateUtils::today(), utils::DateUtils::currentTime(), 15);
    QStringList lines;
    for (const auto& v : soon) {
        const QString id = q(v.appointment.getId());
        if (remindedAppointments_.contains(id)) continue;
        remindedAppointments_.insert(id);
        lines << QString("%1  %2 (%3) - %4")
                     .arg(q(v.appointment.getTime()), q(v.patientName), q(v.category), q(v.appointment.getReason()));
    }
    if (!lines.isEmpty()) {
        auto* box = new QMessageBox(QMessageBox::Information, "Appointment Reminder",
                                    "Starting within 15 minutes:\n\n" + lines.join("\n"), QMessageBox::Ok, this);
        box->setAttribute(Qt::WA_DeleteOnClose);
        box->setModal(false);  // non-blocking: staff can keep working
        box->show();
        showStatus(QString("Reminder: %1 appointment(s) starting soon.").arg(lines.size()), 10000);
    }

    // 2) Waiting-time alerts: patients waiting longer than the threshold.
    const long long now = utils::DateUtils::nowEpoch();
    QStringList waiting;
    for (const auto& e : manager_.database().queueSnapshot()) {
        const long long minutes = (now - e.joinedAt) / 60;
        const QString key = q(e.patientId) + "@" + QString::number(e.joinedAt);
        if (minutes >= QueuePanel::LONG_WAIT_MINUTES && !alertedWaits_.contains(key)) {
            alertedWaits_.insert(key);
            waiting << QString("%1 (%2 min)").arg(q(e.patientId)).arg(minutes);
        }
    }
    if (!waiting.isEmpty()) {
        tabs_->setTabIcon(3, style()->standardIcon(QStyle::SP_MessageBoxWarning));
        statusBar()->setStyleSheet("QStatusBar { color: #B3261E; font-weight: 600; }");
        showStatus("Long wait alert: " + waiting.join(", ") + " waiting over 30 minutes.", 15000);
        QTimer::singleShot(15000, this, [this] { statusBar()->setStyleSheet(QString()); });
    }
    queue_->refreshWaitingTimes();
}

// ===================================================================== file handling

QString MainWindow::defaultDataPath() const {
    // Look next to the executable first, then in common build-folder layouts.
    const QDir appDir(QCoreApplication::applicationDirPath());
    const QStringList candidates = {appDir.filePath("data/clinic_data.txt"), appDir.filePath("../data/clinic_data.txt"),
                                    appDir.filePath("../../data/clinic_data.txt"),
                                    QDir::current().filePath("data/clinic_data.txt")};
    for (const QString& c : candidates) {
        if (QFileInfo::exists(c)) return QFileInfo(c).absoluteFilePath();
    }
    return candidates.first();
}

void MainWindow::loadStartupData() {
    const QString path = defaultDataPath();
    if (QFileInfo::exists(path)) {
        try {
            manager_.load(s(path));
            refreshAll();
            showStatus("Loaded " + path, 6000);
            onReminderTimer();
            return;
        } catch (const utils::ClinicException& e) {
            QMessageBox::warning(this, "Could Not Load Data",
                                 q(e.what()) + "\n\nSample data will be loaded instead.");
        }
    }
    manager_.loadDemoData();
    refreshAll();
    showStatus("Sample data loaded. Use File > Save to keep it.", 8000);
    onReminderTimer();
}

bool MainWindow::maybeSave() {
    if (!manager_.isDirty()) return true;
    const auto answer = QMessageBox::warning(this, "Unsaved Changes", "Do you want to save your changes?",
                                             QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    if (answer == QMessageBox::Save) return saveFile();
    return answer == QMessageBox::Discard;
}

void MainWindow::newSession() {
    if (!maybeSave()) return;
    if (QMessageBox::question(this, "New Session", "Clear all patients, appointments and the queue?") != QMessageBox::Yes)
        return;
    manager_.newSession();
    remindedAppointments_.clear();
    alertedWaits_.clear();
    refreshAll();
    showStatus("New empty session started.");
}

void MainWindow::openFile() {
    if (!maybeSave()) return;
    const QString start = manager_.currentFile().empty() ? QFileInfo(defaultDataPath()).absolutePath()
                                                         : QFileInfo(q(manager_.currentFile())).absolutePath();
    const QString path = QFileDialog::getOpenFileName(this, "Load Clinic Data", start,
                                                      "SCAPMS data (*.txt);;All files (*)");
    if (path.isEmpty()) return;
    if (runGuarded(this, [&] { manager_.load(s(path)); })) {
        remindedAppointments_.clear();
        alertedWaits_.clear();
        refreshAll();
        showStatus("Loaded " + path);
    }
}

bool MainWindow::saveFile() {
    if (manager_.currentFile().empty()) return saveFileAs();
    const QString path = q(manager_.currentFile());
    const bool ok = runGuarded(this, [&] { manager_.save(s(path)); });
    if (ok) {
        refreshAll();
        showStatus("Saved to " + path);
    }
    return ok;
}

bool MainWindow::saveFileAs() {
    const QString start = manager_.currentFile().empty() ? defaultDataPath() : q(manager_.currentFile());
    QDir().mkpath(QFileInfo(start).absolutePath());
    QString path = QFileDialog::getSaveFileName(this, "Save Clinic Data", start, "SCAPMS data (*.txt)");
    if (path.isEmpty()) return false;
    if (!path.endsWith(".txt", Qt::CaseInsensitive)) path += ".txt";
    const bool ok = runGuarded(this, [&] { manager_.save(s(path)); });
    if (ok) {
        refreshAll();
        showStatus("Saved to " + path);
    }
    return ok;
}

void MainWindow::importCsv() {
    const QString path = QFileDialog::getOpenFileName(this, "Import Patients", QString(), "CSV files (*.csv)");
    if (path.isEmpty()) return;
    utils::FileHandler::ImportReport report;
    if (!runGuarded(this, [&] { report = manager_.importCsv(s(path)); })) return;
    refreshAll();
    QString msg = QString("Imported %1 patient(s); skipped %2.").arg(report.imported).arg(report.skipped);
    if (!report.problems.empty()) {
        msg += "\n\nProblems:";
        for (std::size_t i = 0; i < report.problems.size() && i < 10; ++i) msg += "\n- " + q(report.problems[i]);
        if (report.problems.size() > 10) msg += QString("\n... and %1 more").arg(report.problems.size() - 10);
    }
    QMessageBox::information(this, "Import Complete", msg);
}

void MainWindow::exportCsv() {
    const QString path = QFileDialog::getSaveFileName(this, "Export Patients", "patients.csv", "CSV files (*.csv)");
    if (path.isEmpty()) return;
    if (runGuarded(this, [&] { manager_.exportCsv(s(path)); })) showStatus("Exported patients to " + path);
}

void MainWindow::loadSampleData() {
    if (!maybeSave()) return;
    manager_.loadDemoData();
    remindedAppointments_.clear();
    alertedWaits_.clear();
    refreshAll();
    showStatus("Sample data loaded.");
}

void MainWindow::undo() {
    QString what;
    if (runGuarded(this, [&] { what = q(manager_.undoLast()); })) {
        refreshAll();
        showStatus("Undone: " + what, 6000);
    }
}

void MainWindow::focusSearch() {
    tabs_->setCurrentWidget(search_);
    search_->focusSearch();
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (maybeSave()) event->accept();
    else event->ignore();
}

void MainWindow::showShortcuts() {
    QMessageBox::information(this, "Keyboard Shortcuts",
                             "Ctrl+N\tNew session\n"
                             "Ctrl+O\tLoad data file\n"
                             "Ctrl+S\tSave\n"
                             "Ctrl+Shift+S\tSave as\n"
                             "Ctrl+I / Ctrl+E\tImport / export CSV\n"
                             "Ctrl+Z\tUndo last administrative operation\n"
                             "Ctrl+F\tSearch patients\n"
                             "Ctrl+1 ... Ctrl+7\tSwitch tabs\n"
                             "Ctrl+Shift+N\tCall next patient (Queue tab)\n"
                             "Delete\tDelete selected patient (Patients tab)\n"
                             "Esc\tClear patient form\n"
                             "F5\tRefresh\n"
                             "Ctrl+Q\tExit");
}

void MainWindow::showAbout() {
    QMessageBox::about(this, "About SCAPMS",
                       "<h3>Smart Clinic Appointment &amp; Patient Management System</h3>"
                       "<p>COS202 Programming II lab project.</p>"
                       "<p>C++17 with Qt Widgets. Model / Controller / GUI / Utilities layers; "
                       "hand-written searching and sorting algorithms.</p>");
}

} // namespace scapms::gui
