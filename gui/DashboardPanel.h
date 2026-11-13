#pragma once

#include "BasePanel.h"

class QLabel;
class QListWidget;
class QTableWidget;

namespace scapms::gui {

// Dashboard tab: headline statistics, today's upcoming appointments and a
// triage overview produced polymorphically from PatientRecord pointers.
// Refreshed automatically by a timer in the main window.
class DashboardPanel : public BasePanel {
    Q_OBJECT
public:
    explicit DashboardPanel(controller::ClinicManager& manager, QWidget* parent = nullptr);

public slots:
    void refresh() override;

private:
    QLabel* makeCard(const QString& title, const QString& colour, const QString& tooltip, QWidget*& card);

    QLabel* totalValue_;
    QLabel* emergencyValue_;
    QLabel* queueValue_;
    QLabel* todayValue_;
    QLabel* pendingValue_;
    QLabel* completedValue_;
    QTableWidget* upcoming_;
    QListWidget* triage_;
    QLabel* refreshedLabel_;
};

} // namespace scapms::gui
