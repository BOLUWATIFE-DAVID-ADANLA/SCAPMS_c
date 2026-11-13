#include "ReportsPanel.h"
#include "GuiHelpers.h"
#include "../controller/ReportGenerator.h"

#include <QComboBox>
#include <QDateEdit>
#include <QDateTime>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QRegularExpression>
#include <QTableWidget>
#include <QTextStream>
#include <QVBoxLayout>

namespace scapms::gui {

using controller::ReportGenerator;

namespace {
QString bar(double percent) {
    const int blocks = static_cast<int>(percent / 4.0 + 0.5);  // 25 blocks = 100%
    return QString(blocks, QChar(0x2588));
}
} // namespace

ReportsPanel::ReportsPanel(controller::ClinicManager& manager, QWidget* parent) : BasePanel(manager, parent) {
    reportCombo_ = new QComboBox;
    reportCombo_->addItems({"Most Frequently Attended Patients", "Patients with Pending Appointments",
                            "Patient Category Distribution", "Daily Appointment Summary", "Age Group Distribution"});
    reportCombo_->setToolTip("Choose a report; it is generated immediately.");

    dateEdit_ = new QDateEdit(QDate::currentDate());
    dateEdit_->setCalendarPopup(true);
    dateEdit_->setDisplayFormat("yyyy-MM-dd");
    dateEdit_->setToolTip("Day for the Daily Appointment Summary.");

    auto* generateButton = new QPushButton("Generate");
    generateButton->setObjectName("primaryButton");
    generateButton->setToolTip("Rebuild the report from current data.");
    auto* exportButton = new QPushButton("Export...");
    exportButton->setToolTip("Save this report as a text file.");

    auto* controls = new QHBoxLayout;
    controls->addWidget(new QLabel("Report:"));
    controls->addWidget(reportCombo_, 1);
    controls->addWidget(new QLabel("Date:"));
    controls->addWidget(dateEdit_);
    controls->addWidget(generateButton);
    controls->addWidget(exportButton);

    titleLabel_ = new QLabel;
    titleLabel_->setObjectName("sectionTitle");
    summaryLabel_ = new QLabel;
    summaryLabel_->setObjectName("reportSummary");
    summaryLabel_->setWordWrap(true);
    summaryLabel_->setTextFormat(Qt::RichText);

    table_ = new QTableWidget;
    setupTable(table_, {});
    table_->setSelectionMode(QAbstractItemView::NoSelection);

    auto* root = new QVBoxLayout(this);
    root->addLayout(controls);
    root->addWidget(titleLabel_);
    root->addWidget(summaryLabel_);
    root->addWidget(table_, 1);

    connect(reportCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ReportsPanel::generate);
    connect(dateEdit_, &QDateEdit::dateChanged, this, &ReportsPanel::generate);
    connect(generateButton, &QPushButton::clicked, this, &ReportsPanel::generate);
    connect(exportButton, &QPushButton::clicked, this, &ReportsPanel::exportReport);
}

void ReportsPanel::refresh() { generate(); }

void ReportsPanel::setTable(const QStringList& headers) {
    table_->clear();
    table_->setRowCount(0);
    table_->setColumnCount(headers.size());
    table_->setHorizontalHeaderLabels(headers);
    for (int c = 0; c < headers.size(); ++c) table_->setColumnWidth(c, c == 0 ? 120 : 150);
}

void ReportsPanel::addRow(const QStringList& values) {
    const int r = table_->rowCount();
    table_->insertRow(r);
    for (int c = 0; c < values.size(); ++c) table_->setItem(r, c, makeItem(values[c]));
}

void ReportsPanel::generate() {
    const model::ClinicDatabase& db = manager_.database();
    const int type = reportCombo_->currentIndex();
    dateEdit_->setEnabled(type == 3);
    titleLabel_->setText(reportCombo_->currentText());

    switch (type) {
        case 0: {
            const auto rows = ReportGenerator::mostFrequentlyAttended(db, 10);
            setTable({"Rank", "Patient ID", "Name", "Category", "Completed Visits", "Total Appointments"});
            int rank = 0;
            for (const auto& r : rows)
                addRow({QString::number(++rank), q(r.patientId), q(r.name), q(r.category), QString::number(r.completed),
                        QString::number(r.total)});
            summaryLabel_->setText(rows.empty() ? "No completed appointments yet."
                                                : QString("Top %1 patients by completed appointments. Most frequent: "
                                                          "<b>%2</b> with %3 visits.")
                                                      .arg(rows.size())
                                                      .arg(q(rows.front().name))
                                                      .arg(rows.front().completed));
            break;
        }
        case 1: {
            const auto rows = ReportGenerator::pendingAppointments(db);
            setTable({"Patient ID", "Name", "Category", "Appointment", "Date / Time", "Reason"});
            QStringList distinct;
            for (const auto& r : rows) {
                addRow({q(r.patientId), q(r.name), q(r.category), q(r.appointmentId), q(r.dateTime), q(r.reason)});
                if (!distinct.contains(q(r.patientId))) distinct << q(r.patientId);
            }
            summaryLabel_->setText(QString("<b>%1</b> pending appointment(s) across <b>%2</b> patient(s), "
                                           "earliest first.")
                                       .arg(rows.size())
                                       .arg(distinct.size()));
            break;
        }
        case 2: {
            const auto rows = ReportGenerator::categoryDistribution(db);
            setTable({"Category", "Patients", "Share", "Distribution"});
            table_->setColumnWidth(0, 170);
            for (const auto& r : rows)
                addRow({q(r.category), QString::number(r.count), QString::number(r.percent, 'f', 1) + "%",
                        bar(r.percent)});
            const QColor colours[] = {categoryColor(model::PatientCategory::General),
                                      categoryColor(model::PatientCategory::Emergency),
                                      categoryColor(model::PatientCategory::FollowUp)};
            for (int r = 0; r < table_->rowCount(); ++r) table_->item(r, 3)->setForeground(colours[r]);
            summaryLabel_->setText(QString("Total registered patients: <b>%1</b>").arg(db.size()));
            break;
        }
        case 3: {
            const QString date = dateEdit_->date().toString("yyyy-MM-dd");
            const auto sum = ReportGenerator::dailySummary(db, s(date));
            setTable({"Measure", "Count"});
            table_->setColumnWidth(0, 200);
            addRow({"Total Appointments", QString::number(sum.total)});
            addRow({"Completed", QString::number(sum.completed)});
            addRow({"Pending", QString::number(sum.pending)});
            addRow({"Cancelled", QString::number(sum.cancelled)});
            const double rate = sum.total - sum.cancelled > 0 ? 100.0 * sum.completed / (sum.total - sum.cancelled) : 0.0;
            summaryLabel_->setText(QString("Appointments on <b>%1</b>. Completed so far (excluding cancelled): <b>%2%</b>")
                                       .arg(date)
                                       .arg(rate, 0, 'f', 0));
            break;
        }
        case 4: {
            const auto rows = ReportGenerator::ageGroups(db);
            setTable({"Age Group", "Patients", "Distribution"});
            table_->setColumnWidth(0, 200);
            for (const auto& r : rows) {
                const double pct = db.size() ? 100.0 * r.count / db.size() : 0.0;
                addRow({q(r.group), QString::number(r.count), bar(pct)});
            }
            summaryLabel_->setText(QString("Age profile of %1 registered patients.").arg(db.size()));
            break;
        }
        default: break;
    }
}

void ReportsPanel::exportReport() {
    const QString suggested = reportCombo_->currentText().toLower().replace(' ', '_') + ".txt";
    const QString path = QFileDialog::getSaveFileName(this, "Export Report", suggested, "Text files (*.txt)");
    if (path.isEmpty()) return;

    runGuarded(this, [&] {
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
            throw utils::FileException("Cannot write to " + s(path) + ": " + s(file.errorString()));
        QTextStream out(&file);
        out << "SCAPMS Report: " << reportCombo_->currentText() << "\n";
        out << "Generated: " << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm") << "\n";
        QString summary = summaryLabel_->text();
        summary.remove(QRegularExpression("<[^>]*>"));
        out << summary << "\n\n";
        QStringList header;
        for (int c = 0; c < table_->columnCount(); ++c) header << table_->horizontalHeaderItem(c)->text();
        out << header.join(" | ") << "\n" << QString(header.join(" | ").size(), '-') << "\n";
        for (int r = 0; r < table_->rowCount(); ++r) {
            QStringList cells;
            for (int c = 0; c < table_->columnCount(); ++c) cells << (table_->item(r, c) ? table_->item(r, c)->text() : "");
            out << cells.join(" | ") << "\n";
        }
        emit statusMessage("Report exported to " + path);
    });
}

} // namespace scapms::gui
