#pragma once

#include "../model/Appointment.h"
#include "../model/PatientRecord.h"
#include "../utils/ClinicException.h"

#include <QColor>
#include <QHeaderView>
#include <QMessageBox>
#include <QString>
#include <QStringList>
#include <QTableWidget>
#include <QWidget>

#include <exception>
#include <string>

// Small helpers shared by the GUI panels: string conversion, category/status
// colours, table setup and a guarded runner that turns exceptions into dialogs.
namespace scapms::gui {

inline QString q(const std::string& text) { return QString::fromStdString(text); }
inline std::string s(const QString& text) { return text.trimmed().toStdString(); }

inline QColor categoryColor(model::PatientCategory c) {
    switch (c) {
        case model::PatientCategory::Emergency: return QColor("#D93F45");
        case model::PatientCategory::General: return QColor("#3A5CCC");
        case model::PatientCategory::FollowUp: return QColor("#2A9461");
    }
    return QColor("#3A5CCC");
}

inline QColor categoryTint(model::PatientCategory c) {
    switch (c) {
        case model::PatientCategory::Emergency: return QColor("#FDEDEE");
        case model::PatientCategory::General: return QColor("#FFFFFF");
        case model::PatientCategory::FollowUp: return QColor("#EEF8F2");
    }
    return QColor("#FFFFFF");
}

inline QColor statusColor(model::AppointmentStatus st) {
    switch (st) {
        case model::AppointmentStatus::Pending: return QColor("#C77C02");
        case model::AppointmentStatus::Completed: return QColor("#2A9461");
        case model::AppointmentStatus::Cancelled: return QColor("#80838D");
    }
    return QColor("#80838D");
}

inline void setupTable(QTableWidget* table, const QStringList& headers) {
    table->setColumnCount(headers.size());
    table->setHorizontalHeaderLabels(headers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setAlternatingRowColors(false);
    table->verticalHeader()->setVisible(false);
    table->verticalHeader()->setDefaultSectionSize(30);
    table->horizontalHeader()->setStretchLastSection(true);
    table->horizontalHeader()->setHighlightSections(false);
    table->setShowGrid(false);
    table->setWordWrap(false);
}

inline QTableWidgetItem* makeItem(const QString& text, const QVariant& data = QVariant()) {
    auto* item = new QTableWidgetItem(text);
    if (data.isValid()) item->setData(Qt::UserRole, data);
    return item;
}

// Standard patient row used by the Patients and Search panels.
// Columns: ID, Name, Age, Contact, Category, Priority, Next Appointment, Visits
inline QStringList patientHeaders() {
    return {"Patient ID", "Name", "Age", "Contact", "Category", "Priority", "Next Appointment", "Visits"};
}

inline void fillPatientRow(QTableWidget* table, int row, const model::PatientRecord& p) {
    const QColor tint = categoryTint(p.getCategory());
    const std::string next = p.nextAppointmentDateTime();
    const QString visits = QString("%1 done / %2 total")
                               .arg(p.countAppointments(model::AppointmentStatus::Completed))
                               .arg(p.getAppointments().size());
    QTableWidgetItem* items[] = {
        makeItem(q(p.getId()), q(p.getId())),
        makeItem(q(p.getName())),
        makeItem(QString::number(p.getAge())),
        makeItem(q(p.getContact())),
        makeItem(q(p.getCategoryName())),
        makeItem(q(p.getPriorityLabel())),
        makeItem(next.empty() ? QString("-") : q(next)),
        makeItem(visits),
    };
    for (int c = 0; c < 8; ++c) {
        items[c]->setBackground(tint);
        items[c]->setToolTip(q(p.getSummary()));
        table->setItem(row, c, items[c]);
    }
    items[2]->setTextAlignment(Qt::AlignCenter);
    items[4]->setForeground(categoryColor(p.getCategory()));
}

// Runs an action and converts any exception into a meaningful dialog,
// so user errors never terminate the application.
template <typename Action>
bool runGuarded(QWidget* parent, Action&& action) {
    try {
        action();
        return true;
    } catch (const utils::ValidationException& e) {
        QMessageBox::warning(parent, q(e.title()), "Error: " + q(e.what()));
    } catch (const utils::DuplicateIdException& e) {
        QMessageBox::warning(parent, q(e.title()), "Error: " + q(e.what()) +
                                                       "\nChoose a different ID or leave it blank to auto-generate.");
    } catch (const utils::FileException& e) {
        QMessageBox::critical(parent, q(e.title()), q(e.what()));
    } catch (const utils::ClinicException& e) {
        QMessageBox::warning(parent, q(e.title()), q(e.what()));
    } catch (const std::exception& e) {
        QMessageBox::critical(parent, "Unexpected Error",
                              "The operation could not be completed:\n" + QString::fromLocal8Bit(e.what()));
    } catch (...) {
        QMessageBox::critical(parent, "Unexpected Error", "An unknown error occurred. The operation was not completed.");
    }
    return false;
}

} // namespace scapms::gui
