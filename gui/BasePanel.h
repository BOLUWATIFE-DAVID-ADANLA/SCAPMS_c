#pragma once

#include "../controller/ClinicManager.h"

#include <QWidget>

namespace scapms::gui {

// Common base for every tab. Panels talk only to the ClinicManager facade
// and notify the main window through signals (event-driven, loosely coupled).
class BasePanel : public QWidget {
    Q_OBJECT
public:
    explicit BasePanel(controller::ClinicManager& manager, QWidget* parent = nullptr)
        : QWidget(parent), manager_(manager) {}

public slots:
    virtual void refresh() = 0;

signals:
    void dataChanged();                                      // data was modified; others must refresh
    void statusMessage(const QString& message, int timeoutMs = 4000);

protected:
    controller::ClinicManager& manager_;
};

} // namespace scapms::gui
