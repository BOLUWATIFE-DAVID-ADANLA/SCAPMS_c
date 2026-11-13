#pragma once

#include "BasePanel.h"

class QComboBox;
class QDateEdit;
class QLabel;
class QTableWidget;

namespace scapms::gui {

// Reports tab: attendance, pending appointments, category distribution,
// daily summary and age groups. Reports can be exported to a text file.
class ReportsPanel : public BasePanel {
    Q_OBJECT
public:
    explicit ReportsPanel(controller::ClinicManager& manager, QWidget* parent = nullptr);

public slots:
    void refresh() override;

private slots:
    void generate();
    void exportReport();

private:
    void setTable(const QStringList& headers);
    void addRow(const QStringList& values);

    QComboBox* reportCombo_;
    QDateEdit* dateEdit_;
    QLabel* titleLabel_;
    QLabel* summaryLabel_;
    QTableWidget* table_;
};

} // namespace scapms::gui
