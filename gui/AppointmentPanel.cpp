#include "AppointmentPanel.h"
#include "BadgeDelegate.h"
#include "GuiHelpers.h"
#include "../model/Appointable.h"
#include "../model/FollowUpPatient.h"
#include "../utils/DateUtils.h"

#include <QComboBox>
#include <QDateEdit>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTimeEdit>
#include <QVBoxLayout>

namespace scapms::gui {

using model::AppointmentStatus;

AppointmentPanel::AppointmentPanel(controller::ClinicManager& manager, QWidget* parent)
    : BasePanel(manager, parent) {
    // ---------------- booking form (QGridLayout) ----------------
    patientCombo_ = new QComboBox;
    patientCombo_->setMinimumWidth(280);
    patientCombo_->setToolTip("Patient to book. Each patient type has its own booking rules.");
    ruleLabel_ = new QLabel;
    ruleLabel_->setObjectName("hintLabel");
    ruleLabel_->setWordWrap(true);

    dateEdit_ = new QDateEdit(QDate::currentDate().addDays(1));
    dateEdit_->setCalendarPopup(true);
    dateEdit_->setDisplayFormat("yyyy-MM-dd");
    dateEdit_->setMinimumDate(QDate::currentDate());
    dateEdit_->setToolTip("Appointment date (today or later).");
    timeEdit_ = new QTimeEdit(QTime(10, 0));
    timeEdit_->setDisplayFormat("HH:mm");
    timeEdit_->setToolTip("Appointment time, between 08:00 and 18:00.");
    reasonEdit_ = new QLineEdit;
    reasonEdit_->setPlaceholderText("Reason for visit, e.g. Blood pressure check");
    reasonEdit_->setToolTip("Short reason for the appointment (required).");

    auto* scheduleButton = new QPushButton("Schedule Appointment");
    scheduleButton->setObjectName("primaryButton");
    scheduleButton->setToolTip("Book the appointment for the selected patient.");

    auto* formBox = new QGroupBox("Schedule a New Appointment");
    auto* grid = new QGridLayout(formBox);
    grid->addWidget(new QLabel("Patient:"), 0, 0);
    grid->addWidget(patientCombo_, 0, 1, 1, 3);
    grid->addWidget(ruleLabel_, 1, 1, 1, 5);
    grid->addWidget(new QLabel("Date:"), 2, 0);
    grid->addWidget(dateEdit_, 2, 1);
    grid->addWidget(new QLabel("Time:"), 2, 2);
    grid->addWidget(timeEdit_, 2, 3);
    grid->addWidget(new QLabel("Reason:"), 3, 0);
    grid->addWidget(reasonEdit_, 3, 1, 1, 3);
    grid->addWidget(scheduleButton, 3, 4);
    grid->setColumnStretch(5, 1);

    // ---------------- filters + actions (QHBoxLayout) ----------------
    statusFilter_ = new QComboBox;
    statusFilter_->addItems({"All statuses", "Pending", "Completed", "Cancelled"});
    statusFilter_->setToolTip("Show only appointments with this status.");
    rangeFilter_ = new QComboBox;
    rangeFilter_->addItems({"All dates", "Today", "Upcoming", "Past"});
    rangeFilter_->setToolTip("Limit the list to a date range.");

    cancelButton_ = new QPushButton("Cancel Appointment");
    cancelButton_->setToolTip("Cancel the selected pending appointment (undoable).");
    completeButton_ = new QPushButton("Mark Completed");
    completeButton_->setToolTip("Mark the selected appointment as attended (undoable).");
    removeButton_ = new QPushButton("Remove");
    removeButton_->setObjectName("dangerButton");
    removeButton_->setToolTip("Permanently remove the selected appointment record (undoable).");

    auto* bar = new QHBoxLayout;
    bar->addWidget(new QLabel("Show:"));
    bar->addWidget(statusFilter_);
    bar->addWidget(rangeFilter_);
    bar->addStretch();
    bar->addWidget(completeButton_);
    bar->addWidget(cancelButton_);
    bar->addWidget(removeButton_);

    table_ = new QTableWidget;
    setupTable(table_, {"Appt ID", "Patient ID", "Patient", "Category", "Date", "Time", "Reason", "Status"});
    table_->setItemDelegateForColumn(7, BadgeDelegate::forStatus(table_));
    table_->horizontalHeader()->setStretchLastSection(false);
    table_->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Stretch);
    table_->setColumnWidth(0, 80);
    table_->setColumnWidth(1, 85);
    table_->setColumnWidth(2, 170);
    table_->setColumnWidth(3, 90);
    table_->setColumnWidth(4, 95);
    table_->setColumnWidth(5, 60);
    table_->setColumnWidth(7, 120);

    summaryLabel_ = new QLabel;
    summaryLabel_->setObjectName("mutedLabel");

    auto* root = new QVBoxLayout(this);
    root->addWidget(formBox);
    root->addLayout(bar);
    root->addWidget(table_, 1);
    root->addWidget(summaryLabel_);

    connect(patientCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &AppointmentPanel::onPatientChanged);
    connect(scheduleButton, &QPushButton::clicked, this, &AppointmentPanel::schedule);
    connect(reasonEdit_, &QLineEdit::returnPressed, this, &AppointmentPanel::schedule);
    connect(cancelButton_, &QPushButton::clicked, this, &AppointmentPanel::cancelSelected);
    connect(completeButton_, &QPushButton::clicked, this, &AppointmentPanel::completeSelected);
    connect(removeButton_, &QPushButton::clicked, this, &AppointmentPanel::removeSelected);
    connect(statusFilter_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this] { refreshTable(); });
    connect(rangeFilter_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this] { refreshTable(); });
    connect(table_, &QTableWidget::itemSelectionChanged, this, &AppointmentPanel::onTableSelectionChanged);
    connect(table_, &QTableWidget::cellDoubleClicked, this, [this] { completeSelected(); });

    onTableSelectionChanged();
}

void AppointmentPanel::refresh() {
    dateEdit_->setMinimumDate(QDate::currentDate());
    refreshPatients();
    refreshTable();
}

void AppointmentPanel::refreshPatients() {
    const QString keep = patientCombo_->currentData().toString();
    patientCombo_->blockSignals(true);
    patientCombo_->clear();
    for (const auto& p : manager_.database().patients()) {
        patientCombo_->addItem(QString("%1  -  %2  (%3)").arg(q(p->getId()), q(p->getName()), q(p->getCategoryName())),
                               q(p->getId()));
    }
    const int idx = patientCombo_->findData(keep);
    patientCombo_->setCurrentIndex(idx >= 0 ? idx : 0);
    patientCombo_->blockSignals(false);
    onPatientChanged(patientCombo_->currentIndex());
}

void AppointmentPanel::onPatientChanged(int) {
    const QString id = patientCombo_->currentData().toString();
    model::PatientRecord* p = manager_.database().getPatient(s(id));
    if (p == nullptr) {
        ruleLabel_->setText("Register a patient first.");
        return;
    }
    // Use the Appointable interface to describe the booking rules for this type.
    QString rule;
    if (const auto* a = dynamic_cast<const model::Appointable*>(p)) {
        rule = QString("%1 patient: up to %2 pending appointments (currently %3).")
                   .arg(q(p->getCategoryName()))
                   .arg(a->maxPendingAppointments())
                   .arg(p->countAppointments(AppointmentStatus::Pending));
    }
    if (const auto* f = dynamic_cast<const model::FollowUpPatient*>(p); f && !f->getPreviousVisitDate().empty())
        rule += " Must be after the previous visit on " + q(f->getPreviousVisitDate()) + ".";
    ruleLabel_->setText(rule);
}

void AppointmentPanel::refreshTable() {
    const QString keep = table_->currentRow() >= 0 && table_->item(table_->currentRow(), 0)
                             ? table_->item(table_->currentRow(), 0)->text()
                             : QString();
    const std::string today = utils::DateUtils::today();
    const int statusIdx = statusFilter_->currentIndex();
    const int rangeIdx = rangeFilter_->currentIndex();

    table_->blockSignals(true);
    table_->setRowCount(0);
    int pending = 0, completed = 0, cancelled = 0, reselect = -1;
    for (const auto& v : manager_.appointments().all()) {
        const auto& a = v.appointment;
        if (statusIdx == 1 && a.getStatus() != AppointmentStatus::Pending) continue;
        if (statusIdx == 2 && a.getStatus() != AppointmentStatus::Completed) continue;
        if (statusIdx == 3 && a.getStatus() != AppointmentStatus::Cancelled) continue;
        if (rangeIdx == 1 && a.getDate() != today) continue;
        if (rangeIdx == 2 && a.getDate() < today) continue;
        if (rangeIdx == 3 && a.getDate() >= today) continue;

        const int r = table_->rowCount();
        table_->insertRow(r);
        const QString status = q(model::statusToString(a.getStatus()));
        QTableWidgetItem* items[] = {makeItem(q(a.getId())),       makeItem(q(a.getPatientId())),
                                     makeItem(q(v.patientName)),   makeItem(q(v.category)),
                                     makeItem(q(a.getDate())),     makeItem(q(a.getTime())),
                                     makeItem(q(a.getReason())),   makeItem(status)};
        for (int c = 0; c < 8; ++c) table_->setItem(r, c, items[c]);
        if (v.category == "Emergency") items[3]->setForeground(categoryColor(model::PatientCategory::Emergency));
        if (a.getStatus() == AppointmentStatus::Cancelled) {
            for (int c = 0; c < 7; ++c) items[c]->setForeground(QColor("#9A9CA5"));
        }
        if (a.getDate() == today && a.isPending()) items[4]->setText(q(a.getDate()) + " (today)");
        if (q(a.getId()) == keep) reselect = r;

        switch (a.getStatus()) {
            case AppointmentStatus::Pending: ++pending; break;
            case AppointmentStatus::Completed: ++completed; break;
            case AppointmentStatus::Cancelled: ++cancelled; break;
        }
    }
    if (reselect >= 0) table_->selectRow(reselect);
    table_->blockSignals(false);
    summaryLabel_->setText(QString("%1 appointments shown  |  %2 pending  |  %3 completed  |  %4 cancelled")
                               .arg(table_->rowCount())
                               .arg(pending)
                               .arg(completed)
                               .arg(cancelled));
    onTableSelectionChanged();
}

bool AppointmentPanel::selected(QString& patientId, QString& appointmentId) const {
    const int row = table_->currentRow();
    if (row < 0 || table_->selectedItems().isEmpty()) return false;
    appointmentId = table_->item(row, 0)->text();
    patientId = table_->item(row, 1)->text();
    return true;
}

void AppointmentPanel::onTableSelectionChanged() {
    QString pid, aid;
    const bool has = selected(pid, aid);
    const bool pending = has && table_->item(table_->currentRow(), 7)->text() == "Pending";
    cancelButton_->setEnabled(pending);
    completeButton_->setEnabled(pending);
    removeButton_->setEnabled(has);
}

void AppointmentPanel::schedule() {
    const QString id = patientCombo_->currentData().toString();
    if (id.isEmpty()) {
        QMessageBox::warning(this, "No Patient", "Error: Please register a patient before scheduling.");
        return;
    }
    QString apptId;
    const bool ok = runGuarded(this, [&] {
        const auto a = manager_.scheduleAppointment(s(id), s(dateEdit_->date().toString("yyyy-MM-dd")),
                                                    s(timeEdit_->time().toString("HH:mm")), s(reasonEdit_->text()));
        apptId = q(a.getId());
    });
    if (!ok) return;
    reasonEdit_->clear();
    emit dataChanged();
    emit statusMessage(QString("Appointment %1 booked for %2 on %3 at %4.")
                           .arg(apptId, id, dateEdit_->date().toString("yyyy-MM-dd"), timeEdit_->time().toString("HH:mm")));
}

void AppointmentPanel::cancelSelected() {
    QString pid, aid;
    if (!selected(pid, aid)) return;
    if (QMessageBox::question(this, "Cancel Appointment", "Cancel appointment " + aid + " for " + pid + "?") !=
        QMessageBox::Yes)
        return;
    if (runGuarded(this, [&] { manager_.cancelAppointment(s(pid), s(aid)); })) {
        emit dataChanged();
        emit statusMessage("Appointment " + aid + " cancelled. Press Ctrl+Z to undo.");
    }
}

void AppointmentPanel::completeSelected() {
    QString pid, aid;
    if (!selected(pid, aid)) return;
    if (runGuarded(this, [&] { manager_.completeAppointment(s(pid), s(aid)); })) {
        emit dataChanged();
        emit statusMessage("Appointment " + aid + " marked as completed.");
    }
}

void AppointmentPanel::removeSelected() {
    QString pid, aid;
    if (!selected(pid, aid)) return;
    if (QMessageBox::question(this, "Remove Appointment",
                              "Remove appointment " + aid + " from " + pid + "'s record?\nThis can be undone with Ctrl+Z.") !=
        QMessageBox::Yes)
        return;
    if (runGuarded(this, [&] { manager_.removeAppointment(s(pid), s(aid)); })) {
        emit dataChanged();
        emit statusMessage("Appointment " + aid + " removed. Press Ctrl+Z to undo.");
    }
}

} // namespace scapms::gui
