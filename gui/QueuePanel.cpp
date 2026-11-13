#include "QueuePanel.h"
#include "BadgeDelegate.h"
#include "GuiHelpers.h"
#include "../utils/DateUtils.h"

#include <QComboBox>
#include <QDateTime>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

namespace scapms::gui {

QueuePanel::QueuePanel(controller::ClinicManager& manager, QWidget* parent) : BasePanel(manager, parent) {
    // ---------------- left column: controls ----------------
    patientCombo_ = new QComboBox;
    patientCombo_->setToolTip("Patients not already waiting.");
    auto* addButton = new QPushButton("Add to Queue");
    addButton->setToolTip("Patient joins the back of the queue (first come, first served).");

    callButton_ = new QPushButton("Call Next Patient");
    callButton_->setObjectName("primaryButton");
    callButton_->setMinimumHeight(48);
    callButton_->setToolTip("Serve the patient at the front of the queue (Ctrl+Shift+N).");
    callButton_->setShortcut(QKeySequence("Ctrl+Shift+N"));

    nowServingLabel_ = new QLabel("No patient called yet.");
    nowServingLabel_->setObjectName("servingLabel");
    nowServingLabel_->setWordWrap(true);
    nextLabel_ = new QLabel;
    nextLabel_->setObjectName("mutedLabel");
    nextLabel_->setWordWrap(true);

    auto* addBox = new QGroupBox("Join Queue");
    auto* addLayout = new QVBoxLayout(addBox);
    addLayout->addWidget(patientCombo_);
    addLayout->addWidget(addButton);

    auto* serveBox = new QGroupBox("Consultation");
    auto* serveLayout = new QVBoxLayout(serveBox);
    serveLayout->addWidget(callButton_);
    serveLayout->addWidget(new QLabel("Now serving:"));
    serveLayout->addWidget(nowServingLabel_);
    serveLayout->addWidget(nextLabel_);

    auto* left = new QVBoxLayout;
    left->addWidget(addBox);
    left->addWidget(serveBox);
    left->addStretch();

    // ---------------- right column: queue table ----------------
    table_ = new QTableWidget;
    setupTable(table_, {"Position", "Patient ID", "Name", "Category", "Priority", "Joined At", "Waiting"});
    table_->setItemDelegateForColumn(4, BadgeDelegate::forPriority(table_));
    table_->setColumnWidth(0, 70);
    table_->setColumnWidth(1, 85);
    table_->setColumnWidth(2, 170);
    table_->setColumnWidth(3, 90);
    table_->setColumnWidth(4, 130);
    table_->setColumnWidth(5, 85);
    table_->setToolTip("Front of the queue is at the top. Waits over 30 minutes are shown in red.");
    summaryLabel_ = new QLabel;
    summaryLabel_->setObjectName("mutedLabel");

    auto* right = new QVBoxLayout;
    right->addWidget(table_, 1);
    right->addWidget(summaryLabel_);

    auto* root = new QHBoxLayout(this);  // QHBoxLayout: controls beside the table
    auto* leftWidget = new QWidget;
    leftWidget->setLayout(left);
    leftWidget->setFixedWidth(300);
    left->setContentsMargins(0, 0, 0, 0);
    root->addWidget(leftWidget);
    root->addLayout(right, 1);

    connect(addButton, &QPushButton::clicked, this, &QueuePanel::addToQueue);
    connect(callButton_, &QPushButton::clicked, this, &QueuePanel::callNext);
}

void QueuePanel::refresh() {
    refreshPatients();
    refreshWaitingTimes();
}

void QueuePanel::refreshPatients() {
    const QString keep = patientCombo_->currentData().toString();
    patientCombo_->clear();
    for (const auto& p : manager_.database().patients()) {
        if (manager_.database().isQueued(p->getId())) continue;
        patientCombo_->addItem(QString("%1 - %2 (%3)").arg(q(p->getId()), q(p->getName()), q(p->getCategoryName())),
                               q(p->getId()));
    }
    const int idx = patientCombo_->findData(keep);
    if (idx >= 0) patientCombo_->setCurrentIndex(idx);
}

void QueuePanel::refreshWaitingTimes() {
    const auto entries = manager_.database().queueSnapshot();
    const long long now = utils::DateUtils::nowEpoch();
    table_->setRowCount(static_cast<int>(entries.size()));
    int longWaits = 0;
    for (int r = 0; r < static_cast<int>(entries.size()); ++r) {
        const auto& e = entries[r];
        const model::PatientRecord* p = manager_.database().getPatient(e.patientId);
        const long long minutes = (now - e.joinedAt) / 60;
        const bool longWait = minutes >= LONG_WAIT_MINUTES;
        if (longWait) ++longWaits;

        QTableWidgetItem* items[] = {
            makeItem(r == 0 ? "1 (next)" : QString::number(r + 1)),
            makeItem(q(e.patientId)),
            makeItem(p ? q(p->getName()) : "?"),
            makeItem(p ? q(p->getCategoryName()) : "?"),
            makeItem(p ? q(p->getPriorityLabel()) : ""),
            makeItem(QDateTime::fromSecsSinceEpoch(e.joinedAt).toString("HH:mm")),
            makeItem(QString("%1 min").arg(minutes)),
        };
        for (int c = 0; c < 7; ++c) {
            if (p) items[c]->setBackground(categoryTint(p->getCategory()));
            table_->setItem(r, c, items[c]);
        }
        if (r == 0) {
            QFont f = items[0]->font();
            f.setBold(true);
            for (auto* it : items) it->setFont(f);
        }
        if (longWait) {
            items[6]->setForeground(QColor("#D93F45"));
            items[6]->setText(QString("%1 min  ⚠").arg(minutes));
            items[6]->setToolTip("Waiting longer than 30 minutes.");
        }
    }
    callButton_->setEnabled(!entries.empty());
    nextLabel_->setText(entries.empty() ? "The queue is empty."
                                        : "Next up: " + q(entries.front().patientId));
    summaryLabel_->setText(QString("%1 patient(s) waiting%2")
                               .arg(entries.size())
                               .arg(longWaits ? QString("  |  %1 waiting over %2 min").arg(longWaits).arg(LONG_WAIT_MINUTES)
                                              : QString()));
}

void QueuePanel::addToQueue() {
    const QString id = patientCombo_->currentData().toString();
    if (id.isEmpty()) {
        QMessageBox::information(this, "Nobody to Add", "All patients are already in the queue, or none are registered.");
        return;
    }
    if (runGuarded(this, [&] { manager_.addToQueue(s(id)); })) {
        emit dataChanged();
        emit statusMessage(QString("%1 joined the queue at position %2.").arg(id).arg(manager_.database().queueSize()));
    }
}

void QueuePanel::callNext() {
    model::QueueEntry served;
    model::PatientRecord* p = nullptr;
    if (!runGuarded(this, [&] { p = manager_.callNextPatient(served); })) return;

    const long long minutes = (utils::DateUtils::nowEpoch() - served.joinedAt) / 60;
    const QString who = p ? QString("%1 (%2)").arg(q(p->getName()), q(served.patientId)) : q(served.patientId);
    nowServingLabel_->setText(who + (p ? "\n" + q(p->getCategoryName()) + " - " + q(p->getPriorityLabel()) : QString()));
    emit dataChanged();
    emit statusMessage("Now serving " + who);

    QMessageBox::information(this, "Next Patient",
                             QString("Please call:\n\n%1\n\n%2Waited %3 minute(s).")
                                 .arg(who, p ? q(p->getSummary()) + "\n\n" : QString())
                                 .arg(minutes));
}

} // namespace scapms::gui
