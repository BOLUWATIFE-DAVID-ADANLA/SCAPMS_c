#include "PatientPanel.h"
#include "BadgeDelegate.h"
#include "GuiHelpers.h"
#include "../model/EmergencyPatient.h"
#include "../model/FollowUpPatient.h"
#include "../model/GeneralPatient.h"
#include "../utils/Validator.h"

#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSplitter>
#include <QTableWidget>
#include <QVBoxLayout>

namespace scapms::gui {

using model::PatientCategory;

PatientPanel::PatientPanel(controller::ClinicManager& manager, QWidget* parent) : BasePanel(manager, parent) {
    // ---------------- left: table ----------------
    filterEdit_ = new QLineEdit;
    filterEdit_->setPlaceholderText("Filter by ID, name, contact or category...");
    filterEdit_->setClearButtonEnabled(true);
    filterEdit_->setToolTip("Type to filter the table as you type (quick visual filter).");

    table_ = new QTableWidget;
    setupTable(table_, patientHeaders());
    table_->setItemDelegateForColumn(5, BadgeDelegate::forPriority(table_));
    table_->setToolTip("Select a patient to edit. Row colours: red = Emergency, green = Follow-Up.");
    table_->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    table_->setColumnWidth(0, 85);
    table_->setColumnWidth(1, 170);
    table_->setColumnWidth(2, 45);
    table_->setColumnWidth(3, 190);
    table_->setColumnWidth(4, 90);
    table_->setColumnWidth(5, 130);
    table_->setColumnWidth(6, 135);

    countLabel_ = new QLabel;
    countLabel_->setObjectName("mutedLabel");

    auto* left = new QWidget;
    auto* leftLayout = new QVBoxLayout(left);  // QVBoxLayout: filter above table above count
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->addWidget(filterEdit_);
    leftLayout->addWidget(table_, 1);
    leftLayout->addWidget(countLabel_);

    // ---------------- right: form ----------------
    formTitle_ = new QLabel("Register New Patient");
    formTitle_->setObjectName("sectionTitle");

    typeCombo_ = new QComboBox;
    typeCombo_->addItem("General", static_cast<int>(PatientCategory::General));
    typeCombo_->addItem("Emergency", static_cast<int>(PatientCategory::Emergency));
    typeCombo_->addItem("Follow-Up", static_cast<int>(PatientCategory::FollowUp));
    typeCombo_->setToolTip("Patient type. The form shows different fields for each type.");

    idEdit_ = new QLineEdit;
    idEdit_->setMaxLength(12);
    idEdit_->setToolTip("Leave blank to auto-generate (e.g. P0019), or type a unique ID.");
    idStatus_ = new QLabel;
    idStatus_->setObjectName("hintLabel");
    auto* idRow = new QWidget;
    auto* idLayout = new QHBoxLayout(idRow);
    idLayout->setContentsMargins(0, 0, 0, 0);
    idLayout->addWidget(idEdit_, 1);
    idLayout->addWidget(idStatus_);

    nameEdit_ = new QLineEdit;
    nameEdit_->setPlaceholderText("e.g. Adaeze Okafor");
    nameEdit_->setToolTip("Full name: letters, spaces, '-', '.' and apostrophes.");
    ageEdit_ = new QLineEdit;
    ageEdit_->setPlaceholderText("0 - 120");
    ageEdit_->setMaxLength(4);
    ageEdit_->setToolTip("Age in years (0 to 120).");
    contactEdit_ = new QLineEdit;
    contactEdit_->setPlaceholderText("Phone (08031234567) or email");
    contactEdit_->setToolTip("Phone number with 7-15 digits, or an email address.");

    // Type-specific widgets
    departmentCombo_ = new QComboBox;
    departmentCombo_->setEditable(true);
    departmentCombo_->addItems({"General Medicine", "Dental", "Eye Clinic", "Paediatrics", "Physiotherapy"});
    departmentCombo_->setToolTip("Department handling this routine patient.");
    insuranceEdit_ = new QLineEdit;
    insuranceEdit_->setPlaceholderText("Optional, e.g. NHIS-44120");
    severityCombo_ = new QComboBox;
    for (int i = 1; i <= 5; ++i) {
        static const char* labels[] = {"", "Minor", "Moderate", "Serious", "Severe", "Critical"};
        severityCombo_->addItem(QString("%1 - %2").arg(i).arg(labels[i]), i);
    }
    severityCombo_->setCurrentIndex(2);
    severityCombo_->setToolTip("Triage severity: 4 and 5 are flagged as critical.");
    conditionEdit_ = new QLineEdit;
    conditionEdit_->setPlaceholderText("Presenting condition");
    previousVisitEdit_ = new QLineEdit;
    previousVisitEdit_->setPlaceholderText("YYYY-MM-DD");
    previousVisitEdit_->setToolTip("Date of the visit being followed up. New appointments must be after it.");
    doctorEdit_ = new QLineEdit;
    doctorEdit_->setPlaceholderText("e.g. Dr. Adeyemi");

    auto makeExtra = [](const QString& label, QWidget* field) {
        auto* l = new QLabel(label);
        l->hide();
        field->hide();
        return ExtraField{l, field};
    };
    generalFields_ = {makeExtra("Department:", departmentCombo_), makeExtra("Insurance ID:", insuranceEdit_)};
    emergencyFields_ = {makeExtra("Severity:", severityCombo_), makeExtra("Condition:", conditionEdit_)};
    followUpFields_ = {makeExtra("Previous visit:", previousVisitEdit_), makeExtra("Referring doctor:", doctorEdit_)};

    auto* formBox = new QGroupBox("Patient Details");
    form_ = new QFormLayout(formBox);  // QFormLayout: label/field pairs aligned automatically
    form_->setLabelAlignment(Qt::AlignRight);
    form_->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    form_->addRow("Patient type:", typeCombo_);
    form_->addRow("Patient ID:", idRow);
    form_->addRow("Full name:", nameEdit_);
    form_->addRow("Age:", ageEdit_);
    form_->addRow("Contact:", contactEdit_);

    addButton_ = new QPushButton("Add Patient");
    addButton_->setObjectName("primaryButton");
    addButton_->setToolTip("Register a new patient with these details (Enter).");
    updateButton_ = new QPushButton("Save Changes");
    updateButton_->setToolTip("Update the selected patient. Undo with Ctrl+Z.");
    deleteButton_ = new QPushButton("Delete");
    deleteButton_->setObjectName("dangerButton");
    deleteButton_->setToolTip("Delete the selected patient (Del). Undo with Ctrl+Z.");
    deleteButton_->setShortcut(QKeySequence::Delete);
    auto* clearButton = new QPushButton("Clear");
    clearButton->setToolTip("Clear the form and deselect (Esc).");
    clearButton->setShortcut(Qt::Key_Escape);
    queueButton_ = new QPushButton("Add to Waiting Queue");
    queueButton_->setToolTip("Put the selected patient at the back of the consultation queue.");

    auto* buttonGrid = new QGridLayout;  // QGridLayout: 2x2 button block
    buttonGrid->addWidget(addButton_, 0, 0);
    buttonGrid->addWidget(updateButton_, 0, 1);
    buttonGrid->addWidget(deleteButton_, 1, 0);
    buttonGrid->addWidget(clearButton, 1, 1);
    buttonGrid->addWidget(queueButton_, 2, 0, 1, 2);

    auto* right = new QWidget;
    auto* rightLayout = new QVBoxLayout(right);
    rightLayout->setContentsMargins(8, 0, 0, 0);
    rightLayout->addWidget(formTitle_);
    rightLayout->addWidget(formBox);
    rightLayout->addLayout(buttonGrid);
    rightLayout->addStretch();
    right->setMinimumWidth(340);
    right->setMaximumWidth(440);

    auto* splitter = new QSplitter(Qt::Horizontal);
    splitter->addWidget(left);
    splitter->addWidget(right);
    splitter->setStretchFactor(0, 1);
    splitter->setChildrenCollapsible(false);

    auto* root = new QHBoxLayout(this);
    root->addWidget(splitter);

    // ---------------- events ----------------
    connect(typeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PatientPanel::onTypeChanged);
    connect(idEdit_, &QLineEdit::textChanged, this, &PatientPanel::onIdEdited);
    connect(filterEdit_, &QLineEdit::textChanged, this, &PatientPanel::onFilterChanged);
    connect(table_, &QTableWidget::itemSelectionChanged, this, &PatientPanel::onSelectionChanged);
    connect(addButton_, &QPushButton::clicked, this, &PatientPanel::addPatient);
    connect(updateButton_, &QPushButton::clicked, this, &PatientPanel::updatePatient);
    connect(deleteButton_, &QPushButton::clicked, this, &PatientPanel::deletePatient);
    connect(clearButton, &QPushButton::clicked, this, &PatientPanel::clearForm);
    connect(queueButton_, &QPushButton::clicked, this, &PatientPanel::addToQueue);
    for (QLineEdit* e : {nameEdit_, ageEdit_, contactEdit_}) {
        connect(e, &QLineEdit::returnPressed, this, [this] {
            if (editingId_.isEmpty()) addPatient();
            else updatePatient();
        });
    }

    showExtrasFor(PatientCategory::General);
    clearForm();
}

// ------------------------------------------------------------------ dynamic fields

void PatientPanel::showExtrasFor(PatientCategory category) {
    // Remove the currently displayed type-specific rows from the form...
    if (activeExtras_ != nullptr) {
        for (const ExtraField& f : *activeExtras_) {
            QFormLayout::TakeRowResult taken = form_->takeRow(f.field);
            delete taken.labelItem;  // deletes the layout wrapper only, not the widget
            delete taken.fieldItem;
            f.label->hide();
            f.field->hide();
        }
    }
    // ...then add the rows that belong to the newly selected type.
    switch (category) {
        case PatientCategory::General: activeExtras_ = &generalFields_; break;
        case PatientCategory::Emergency: activeExtras_ = &emergencyFields_; break;
        case PatientCategory::FollowUp: activeExtras_ = &followUpFields_; break;
    }
    for (const ExtraField& f : *activeExtras_) {
        form_->addRow(f.label, f.field);
        f.label->show();
        f.field->show();
    }
}

void PatientPanel::onTypeChanged(int index) {
    showExtrasFor(static_cast<PatientCategory>(typeCombo_->itemData(index).toInt()));
}

void PatientPanel::onIdEdited(const QString& text) {
    const QString id = text.trimmed();
    if (id.isEmpty()) {
        idStatus_->setText(editingId_.isEmpty() ? "auto: " + q(manager_.suggestPatientId()) : "unchanged");
        idStatus_->setStyleSheet("color:#80838D;");
        return;
    }
    if (id != editingId_ && manager_.database().containsId(s(id))) {
        idStatus_->setText("✗ in use");
        idStatus_->setStyleSheet("color:#D93F45; font-weight:600;");
        return;
    }
    try {
        utils::Validator::validatePatientId(s(id));
        idStatus_->setText("✓ available");
        idStatus_->setStyleSheet("color:#2A9461; font-weight:600;");
    } catch (const utils::ValidationException&) {
        idStatus_->setText("✗ invalid");
        idStatus_->setStyleSheet("color:#D93F45; font-weight:600;");
    }
}

void PatientPanel::onFilterChanged(const QString& text) {
    const QString needle = text.trimmed();
    int visible = 0;
    for (int r = 0; r < table_->rowCount(); ++r) {
        bool match = needle.isEmpty();
        for (int c : {0, 1, 3, 4}) {
            if (!match && table_->item(r, c)->text().contains(needle, Qt::CaseInsensitive)) match = true;
        }
        table_->setRowHidden(r, !match);
        if (match) ++visible;
    }
    countLabel_->setText(QString("Showing %1 of %2 patients").arg(visible).arg(table_->rowCount()));
}

// ------------------------------------------------------------------ table

void PatientPanel::refresh() {
    const QString keep = selectedId().isEmpty() ? editingId_ : selectedId();
    table_->blockSignals(true);
    table_->setRowCount(0);
    const auto& patients = manager_.database().patients();
    table_->setRowCount(static_cast<int>(patients.size()));
    int reselect = -1;
    for (int r = 0; r < static_cast<int>(patients.size()); ++r) {
        fillPatientRow(table_, r, *patients[r]);
        if (q(patients[r]->getId()) == keep) reselect = r;
    }
    if (reselect >= 0) table_->selectRow(reselect);
    table_->blockSignals(false);
    if (reselect < 0 && !editingId_.isEmpty() && !manager_.database().containsId(s(editingId_))) clearForm();
    onFilterChanged(filterEdit_->text());
    onIdEdited(idEdit_->text());
}

QString PatientPanel::selectedId() const {
    const auto rows = table_->selectionModel() ? table_->selectionModel()->selectedRows() : QModelIndexList();
    if (rows.isEmpty()) return {};
    return table_->item(rows.first().row(), 0)->data(Qt::UserRole).toString();
}

void PatientPanel::onSelectionChanged() {
    const QString id = selectedId();
    if (id.isEmpty()) return;
    model::PatientRecord* p = manager_.findPatient(s(id));  // also records access in the array cache
    if (p == nullptr) return;

    editingId_ = id;
    formTitle_->setText("Edit Patient " + id);
    typeCombo_->setCurrentIndex(typeCombo_->findData(static_cast<int>(p->getCategory())));
    idEdit_->setText(q(p->getId()));
    nameEdit_->setText(q(p->getName()));
    ageEdit_->setText(QString::number(p->getAge()));
    contactEdit_->setText(q(p->getContact()));
    setExtraValues(*p);
    addButton_->setEnabled(false);
    updateButton_->setEnabled(true);
    deleteButton_->setEnabled(true);
    queueButton_->setEnabled(!manager_.database().isQueued(s(id)));
    queueButton_->setText(manager_.database().isQueued(s(id)) ? "Already in Queue" : "Add to Waiting Queue");
    emit statusMessage(q(controller::ClinicManager::processPatient(*p)), 6000);
}

void PatientPanel::setExtraValues(const model::PatientRecord& p) {
    // dynamic_cast picks the concrete type to fill the matching widgets.
    if (const auto* g = dynamic_cast<const model::GeneralPatient*>(&p)) {
        departmentCombo_->setCurrentText(q(g->getDepartment()));
        insuranceEdit_->setText(q(g->getInsuranceId()));
    } else if (const auto* e = dynamic_cast<const model::EmergencyPatient*>(&p)) {
        severityCombo_->setCurrentIndex(severityCombo_->findData(e->getSeverity()));
        conditionEdit_->setText(q(e->getCondition()));
    } else if (const auto* f = dynamic_cast<const model::FollowUpPatient*>(&p)) {
        previousVisitEdit_->setText(q(f->getPreviousVisitDate()));
        doctorEdit_->setText(q(f->getReferringDoctor()));
    }
}

controller::PatientInput PatientPanel::readForm() const {
    controller::PatientInput in;
    in.category = static_cast<PatientCategory>(typeCombo_->currentData().toInt());
    in.id = s(idEdit_->text());
    in.name = s(nameEdit_->text());
    in.age = s(ageEdit_->text());
    in.contact = s(contactEdit_->text());
    switch (in.category) {
        case PatientCategory::General:
            in.extra1 = s(departmentCombo_->currentText());
            in.extra2 = s(insuranceEdit_->text());
            break;
        case PatientCategory::Emergency:
            in.extra1 = std::to_string(severityCombo_->currentData().toInt());
            in.extra2 = s(conditionEdit_->text());
            break;
        case PatientCategory::FollowUp:
            in.extra1 = s(previousVisitEdit_->text());
            in.extra2 = s(doctorEdit_->text());
            break;
    }
    return in;
}

// ------------------------------------------------------------------ actions

void PatientPanel::addPatient() {
    QString newId;
    const bool ok = runGuarded(this, [&] {
        const model::PatientRecord& p = manager_.registerPatient(readForm());
        newId = q(p.getId());
    });
    if (!ok) return;
    clearForm();
    emit dataChanged();
    emit statusMessage("Patient " + newId + " registered.");
}

void PatientPanel::updatePatient() {
    if (editingId_.isEmpty()) {
        QMessageBox::information(this, "No Patient Selected", "Select a patient in the table to edit.");
        return;
    }
    const controller::PatientInput input = readForm();
    if (runGuarded(this, [&] { manager_.updatePatient(s(editingId_), input); })) {
        editingId_ = input.id.empty() ? editingId_ : q(input.id);
        emit dataChanged();
        emit statusMessage("Patient " + editingId_ + " updated. Press Ctrl+Z to undo.");
    }
}

void PatientPanel::deletePatient() {
    const QString id = editingId_;
    if (id.isEmpty()) {
        QMessageBox::information(this, "No Patient Selected", "Select a patient in the table to delete.");
        return;
    }
    const auto answer = QMessageBox::question(
        this, "Delete Patient",
        QString("Delete patient %1 (%2) and all their appointments?\n\nYou can reverse this with Edit > Undo (Ctrl+Z).")
            .arg(id, nameEdit_->text()));
    if (answer != QMessageBox::Yes) return;
    if (runGuarded(this, [&] { manager_.deletePatient(s(id)); })) {
        clearForm();
        emit dataChanged();
        emit statusMessage("Patient " + id + " deleted. Press Ctrl+Z to undo.", 6000);
    }
}

void PatientPanel::addToQueue() {
    const QString id = editingId_;
    if (id.isEmpty()) return;
    if (runGuarded(this, [&] { manager_.addToQueue(s(id)); })) {
        queueButton_->setEnabled(false);
        queueButton_->setText("Already in Queue");
        emit dataChanged();
        emit statusMessage(QString("%1 added to the waiting queue (position %2).")
                               .arg(id)
                               .arg(manager_.database().queueSize()));
    }
}

void PatientPanel::clearForm() {
    editingId_.clear();
    table_->clearSelection();
    formTitle_->setText("Register New Patient");
    for (QLineEdit* e : {idEdit_, nameEdit_, ageEdit_, contactEdit_, insuranceEdit_, conditionEdit_,
                         previousVisitEdit_, doctorEdit_}) {
        e->clear();
    }
    departmentCombo_->setCurrentIndex(0);
    severityCombo_->setCurrentIndex(2);
    addButton_->setEnabled(true);
    updateButton_->setEnabled(false);
    deleteButton_->setEnabled(false);
    queueButton_->setEnabled(false);
    queueButton_->setText("Add to Waiting Queue");
    onIdEdited(QString());
    nameEdit_->setFocus();
}

} // namespace scapms::gui
