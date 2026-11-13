#pragma once

#include "BasePanel.h"

class QLabel;
class QListWidget;
class QPushButton;

namespace scapms::gui {

// Admin tab: undo stack (std::stack), recently accessed patient cache
// (fixed-size array) and data management actions.
class AdminPanel : public BasePanel {
    Q_OBJECT
public:
    explicit AdminPanel(controller::ClinicManager& manager, QWidget* parent = nullptr);

public slots:
    void refresh() override;

signals:
    void undoRequested();
    void newSessionRequested();
    void sampleDataRequested();

private:
    QListWidget* undoList_;
    QPushButton* undoButton_;
    QListWidget* cacheList_;
    QLabel* cacheLabel_;
    QLabel* statsLabel_;
};

} // namespace scapms::gui
