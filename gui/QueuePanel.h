#pragma once

#include "BasePanel.h"

class QComboBox;
class QLabel;
class QPushButton;
class QTableWidget;

namespace scapms::gui {

// Waiting Queue tab: FIFO queue backed by std::queue.
class QueuePanel : public BasePanel {
    Q_OBJECT
public:
    explicit QueuePanel(controller::ClinicManager& manager, QWidget* parent = nullptr);

    static constexpr int LONG_WAIT_MINUTES = 30;

public slots:
    void refresh() override;
    void refreshWaitingTimes();  // called by the main window's timer

private slots:
    void addToQueue();
    void callNext();

private:
    void refreshPatients();

    QComboBox* patientCombo_;
    QPushButton* callButton_;
    QLabel* nowServingLabel_;
    QLabel* nextLabel_;
    QTableWidget* table_;
    QLabel* summaryLabel_;
};

} // namespace scapms::gui
