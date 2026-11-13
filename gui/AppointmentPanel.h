#pragma once

#include "BasePanel.h"

class QComboBox;
class QDateEdit;
class QLabel;
class QLineEdit;
class QPushButton;
class QTableWidget;
class QTimeEdit;

namespace scapms::gui {

// Appointments tab: schedule, cancel, complete, remove and view appointments.
class AppointmentPanel : public BasePanel {
    Q_OBJECT
public:
    explicit AppointmentPanel(controller::ClinicManager& manager, QWidget* parent = nullptr);

public slots:
    void refresh() override;

private slots:
    void onPatientChanged(int index);
    void onTableSelectionChanged();
    void schedule();
    void cancelSelected();
    void completeSelected();
    void removeSelected();

private:
    void refreshPatients();
    void refreshTable();
    bool selected(QString& patientId, QString& appointmentId) const;

    QComboBox* patientCombo_;
    QLabel* ruleLabel_;
    QDateEdit* dateEdit_;
    QTimeEdit* timeEdit_;
    QLineEdit* reasonEdit_;
    QComboBox* statusFilter_;
    QComboBox* rangeFilter_;
    QTableWidget* table_;
    QPushButton* cancelButton_;
    QPushButton* completeButton_;
    QPushButton* removeButton_;
    QLabel* summaryLabel_;
};

} // namespace scapms::gui
