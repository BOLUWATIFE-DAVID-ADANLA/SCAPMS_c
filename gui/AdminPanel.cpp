#include "AdminPanel.h"
#include "GuiHelpers.h"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

namespace scapms::gui {

AdminPanel::AdminPanel(controller::ClinicManager& manager, QWidget* parent) : BasePanel(manager, parent) {
    // ---- Undo stack ----
    undoList_ = new QListWidget;
    undoList_->setToolTip("Most recent operation is at the top (Last In, First Out).");
    undoButton_ = new QPushButton("Undo Last Operation");
    undoButton_->setObjectName("primaryButton");
    undoButton_->setToolTip("Reverse the most recent administrative operation (Ctrl+Z).");
    auto* undoBox = new QGroupBox("Undo Stack (std::stack)");
    auto* ul = new QVBoxLayout(undoBox);
    auto* undoHint = new QLabel("Deleting or modifying patients and cancelling, completing or removing appointments "
                                "are pushed here. Undo pops the top entry.");
    undoHint->setWordWrap(true);
    undoHint->setObjectName("mutedLabel");
    ul->addWidget(undoHint);
    ul->addWidget(undoList_, 1);
    ul->addWidget(undoButton_);

    // ---- Cache ----
    cacheList_ = new QListWidget;
    cacheList_->setToolTip("PatientRecord* recentPatients[10] - most recently accessed first.");
    cacheLabel_ = new QLabel;
    cacheLabel_->setObjectName("mutedLabel");
    cacheLabel_->setWordWrap(true);
    auto* cacheBox = new QGroupBox("Frequently Accessed Patients (array cache)");
    auto* cl = new QVBoxLayout(cacheBox);
    cl->addWidget(cacheLabel_);
    cl->addWidget(cacheList_, 1);

    // ---- Data management ----
    statsLabel_ = new QLabel;
    statsLabel_->setWordWrap(true);
    auto* sampleButton = new QPushButton("Load Sample Data");
    sampleButton->setToolTip("Replace current data with the built-in demo data set.");
    auto* clearButton = new QPushButton("Clear All Data");
    clearButton->setObjectName("dangerButton");
    clearButton->setToolTip("Start a new, empty session (same as File > New).");
    auto* dataBox = new QGroupBox("Data Management");
    auto* dl = new QVBoxLayout(dataBox);
    dl->addWidget(statsLabel_);
    dl->addStretch();
    dl->addWidget(sampleButton);
    dl->addWidget(clearButton);

    auto* root = new QHBoxLayout(this);
    root->addWidget(undoBox, 3);
    root->addWidget(cacheBox, 3);
    root->addWidget(dataBox, 2);

    connect(undoButton_, &QPushButton::clicked, this, &AdminPanel::undoRequested);
    connect(sampleButton, &QPushButton::clicked, this, &AdminPanel::sampleDataRequested);
    connect(clearButton, &QPushButton::clicked, this, &AdminPanel::newSessionRequested);
}

void AdminPanel::refresh() {
    undoList_->clear();
    const auto history = manager_.undoHistory();
    for (std::size_t i = 0; i < history.size(); ++i)
        undoList_->addItem((i == 0 ? QString("TOP  ") : QString("     ")) + q(history[i]));
    if (history.empty()) undoList_->addItem("(nothing to undo)");
    undoButton_->setEnabled(manager_.canUndo());

    cacheList_->clear();
    const auto cached = manager_.database().cachedPatients();
    for (std::size_t i = 0; i < cached.size(); ++i) {
        auto* item = new QListWidgetItem(QString("[%1]  %2  -  %3 (%4)")
                                             .arg(i)
                                             .arg(q(cached[i]->getId()), q(cached[i]->getName()),
                                                  q(cached[i]->getCategoryName())));
        item->setForeground(categoryColor(cached[i]->getCategory()));
        cacheList_->addItem(item);
    }
    for (int i = static_cast<int>(cached.size()); i < model::ClinicDatabase::CACHE_SIZE; ++i)
        cacheList_->addItem(QString("[%1]  (empty)").arg(i));
    cacheLabel_->setText(QString("%1 of %2 slots used. Selecting or booking a patient moves them to slot 0; "
                                 "the least recently used patient drops out when full.")
                             .arg(cached.size())
                             .arg(model::ClinicDatabase::CACHE_SIZE));

    const auto& db = manager_.database();
    int appts = 0;
    for (const auto& p : db.patients()) appts += static_cast<int>(p->getAppointments().size());
    statsLabel_->setText(QString("<b>Patients:</b> %1<br><b>Appointments:</b> %2<br><b>In queue:</b> %3<br>"
                                 "<b>Undo entries:</b> %4<br><b>Data file:</b> %5<br><b>Unsaved changes:</b> %6")
                             .arg(db.size())
                             .arg(appts)
                             .arg(db.queueSize())
                             .arg(history.size())
                             .arg(manager_.currentFile().empty() ? QString("(not saved yet)")
                                                                 : q(manager_.currentFile()).toHtmlEscaped())
                             .arg(manager_.isDirty() ? "Yes" : "No"));
}

} // namespace scapms::gui
