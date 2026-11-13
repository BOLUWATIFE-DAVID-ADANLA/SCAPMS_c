#pragma once

#include "BasePanel.h"

#include <QVector>

class QComboBox;
class QFormLayout;
class QLabel;
class QLineEdit;
class QPushButton;
class QTableWidget;

namespace scapms::gui {

// Patient Records tab: view, register, edit and delete patients.
// The form adds/removes type-specific fields when the patient type changes.
class PatientPanel : public BasePanel {
    Q_OBJECT
public:
    explicit PatientPanel(controller::ClinicManager& manager, QWidget* parent = nullptr);

public slots:
    void refresh() override;

private slots:
    void onTypeChanged(int index);
    void onIdEdited(const QString& text);
    void onFilterChanged(const QString& text);
    void onSelectionChanged();
    void addPatient();
    void updatePatient();
    void deletePatient();
    void addToQueue();
    void clearForm();

private:
    struct ExtraField {
        QLabel* label;
        QWidget* field;
    };

    controller::PatientInput readForm() const;
    void showExtrasFor(model::PatientCategory category);
    void setExtraValues(const model::PatientRecord& patient);
    QString selectedId() const;

    QLineEdit* filterEdit_;
    QTableWidget* table_;
    QLabel* countLabel_;

    QFormLayout* form_;
    QComboBox* typeCombo_;
    QLineEdit* idEdit_;
    QLabel* idStatus_;
    QLineEdit* nameEdit_;
    QLineEdit* ageEdit_;
    QLineEdit* contactEdit_;

    // Type-specific widgets (created once, inserted into / removed from the form on demand)
    QComboBox* departmentCombo_;
    QLineEdit* insuranceEdit_;
    QComboBox* severityCombo_;
    QLineEdit* conditionEdit_;
    QLineEdit* previousVisitEdit_;
    QLineEdit* doctorEdit_;
    QVector<ExtraField> generalFields_, emergencyFields_, followUpFields_;
    QVector<ExtraField>* activeExtras_ = nullptr;

    QLabel* formTitle_;
    QPushButton* addButton_;
    QPushButton* updateButton_;
    QPushButton* deleteButton_;
    QPushButton* queueButton_;
    QString editingId_;
};

} // namespace scapms::gui
