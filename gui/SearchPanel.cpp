#include "SearchPanel.h"
#include "BadgeDelegate.h"
#include "GuiHelpers.h"

#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTextBrowser>
#include <QTimer>
#include <QVBoxLayout>

namespace scapms::gui {

using controller::SearchAlgorithm;
using controller::SearchField;
using controller::SortAlgorithm;
using controller::SortEngine;
using model::SortField;

SearchPanel::SearchPanel(controller::ClinicManager& manager, QWidget* parent) : BasePanel(manager, parent) {
    // ---------------- search box ----------------
    keywordEdit_ = new QLineEdit;
    keywordEdit_->setPlaceholderText("Enter Patient ID, name or category...");
    keywordEdit_->setClearButtonEnabled(true);
    keywordEdit_->setToolTip("Search keyword (Ctrl+F to jump here).");

    fieldCombo_ = new QComboBox;
    fieldCombo_->addItem("Patient ID", static_cast<int>(SearchField::Id));
    fieldCombo_->addItem("Patient Name", static_cast<int>(SearchField::Name));
    fieldCombo_->addItem("Category", static_cast<int>(SearchField::Category));
    fieldCombo_->setCurrentIndex(1);
    fieldCombo_->setToolTip("Which field to search.");

    searchAlgoCombo_ = new QComboBox;
    searchAlgoCombo_->addItem("Auto (based on data state)", static_cast<int>(SearchAlgorithm::Auto));
    searchAlgoCombo_->addItem("Linear Search", static_cast<int>(SearchAlgorithm::Linear));
    searchAlgoCombo_->addItem("Binary Search", static_cast<int>(SearchAlgorithm::Binary));
    searchAlgoCombo_->addItem("Recursive Search", static_cast<int>(SearchAlgorithm::Recursive));
    searchAlgoCombo_->setToolTip("Auto uses binary search when the data is sorted by the searched field, "
                                 "otherwise linear or recursive search.");

    liveCheck_ = new QCheckBox("Search as I type");
    liveCheck_->setChecked(true);
    liveCheck_->setToolTip("Re-run the search shortly after each keystroke.");

    auto* searchButton = new QPushButton("Search");
    searchButton->setObjectName("primaryButton");
    searchButton->setToolTip("Run the search (Enter).");
    auto* showAllButton = new QPushButton("Show All");
    showAllButton->setToolTip("Clear the search and list every patient in the current order.");

    auto* searchBox = new QGroupBox("Search Patients");
    auto* sg = new QGridLayout(searchBox);
    sg->addWidget(new QLabel("Keyword:"), 0, 0);
    sg->addWidget(keywordEdit_, 0, 1, 1, 3);
    sg->addWidget(new QLabel("Search by:"), 1, 0);
    sg->addWidget(fieldCombo_, 1, 1);
    sg->addWidget(new QLabel("Algorithm:"), 1, 2);
    sg->addWidget(searchAlgoCombo_, 1, 3);
    sg->addWidget(liveCheck_, 2, 1);
    auto* sb = new QHBoxLayout;
    sb->addStretch();
    sb->addWidget(showAllButton);
    sb->addWidget(searchButton);
    sg->addLayout(sb, 2, 2, 1, 2);
    sg->setColumnStretch(1, 1);
    sg->setColumnStretch(3, 1);

    // ---------------- sort box ----------------
    sortAlgoCombo_ = new QComboBox;
    sortAlgoCombo_->addItem("Selection Sort", static_cast<int>(SortAlgorithm::Selection));
    sortAlgoCombo_->addItem("Insertion Sort", static_cast<int>(SortAlgorithm::Insertion));
    sortAlgoCombo_->addItem("Merge Sort", static_cast<int>(SortAlgorithm::Merge));
    sortAlgoCombo_->addItem("Quick Sort", static_cast<int>(SortAlgorithm::Quick));
    sortAlgoCombo_->setToolTip("Hand-written sorting algorithm to use.");

    sortFieldCombo_ = new QComboBox;
    sortFieldCombo_->addItem("Patient Name", static_cast<int>(SortField::Name));
    sortFieldCombo_->addItem("Patient ID", static_cast<int>(SortField::Id));
    sortFieldCombo_->addItem("Age", static_cast<int>(SortField::Age));
    sortFieldCombo_->addItem("Next Appointment Date", static_cast<int>(SortField::AppointmentDate));
    sortFieldCombo_->setToolTip("Field to sort the patient records by.");

    orderCombo_ = new QComboBox;
    orderCombo_->addItems({"Ascending", "Descending"});
    orderCombo_->setToolTip("Binary search needs ascending order.");

    auto* sortButton = new QPushButton("SORT");
    sortButton->setObjectName("primaryButton");
    sortButton->setToolTip("Sort all patient records with the selected algorithm.");
    auto* compareButton = new QPushButton("Compare Algorithms");
    compareButton->setToolTip("Run all four algorithms on a copy of the data and compare their work.");

    auto* sortBox = new QGroupBox("Sort Patients");
    auto* so = new QGridLayout(sortBox);
    so->addWidget(new QLabel("Sort Algorithm:"), 0, 0);
    so->addWidget(sortAlgoCombo_, 0, 1);
    so->addWidget(new QLabel("Sort By:"), 1, 0);
    so->addWidget(sortFieldCombo_, 1, 1);
    so->addWidget(new QLabel("Order:"), 2, 0);
    so->addWidget(orderCombo_, 2, 1);
    auto* sob = new QHBoxLayout;
    sob->addWidget(compareButton);
    sob->addWidget(sortButton);
    so->addLayout(sob, 3, 0, 1, 2);

    auto* top = new QHBoxLayout;
    top->addWidget(searchBox, 3);
    top->addWidget(sortBox, 2);

    // ---------------- info + results ----------------
    stateLabel_ = new QLabel;
    stateLabel_->setObjectName("stateLabel");
    info_ = new QTextBrowser;
    info_->setMaximumHeight(120);
    info_->setOpenLinks(false);
    info_->setToolTip("Details of the last search or sort, including why the algorithm fits.");

    results_ = new QTableWidget;
    setupTable(results_, patientHeaders());
    results_->setItemDelegateForColumn(5, BadgeDelegate::forPriority(results_));
    results_->setColumnWidth(0, 85);
    results_->setColumnWidth(1, 170);
    results_->setColumnWidth(2, 45);
    results_->setColumnWidth(3, 190);
    results_->setColumnWidth(4, 90);
    results_->setColumnWidth(5, 130);
    results_->setColumnWidth(6, 135);
    resultCount_ = new QLabel;
    resultCount_->setObjectName("mutedLabel");

    auto* root = new QVBoxLayout(this);
    root->addLayout(top);
    root->addWidget(stateLabel_);
    root->addWidget(info_);
    root->addWidget(results_, 1);
    root->addWidget(resultCount_);

    // ---------------- events ----------------
    debounce_ = new QTimer(this);
    debounce_->setSingleShot(true);
    debounce_->setInterval(300);
    connect(debounce_, &QTimer::timeout, this, &SearchPanel::runLiveSearch);
    connect(keywordEdit_, &QLineEdit::textChanged, this, [this](const QString& text) {
        if (!liveCheck_->isChecked()) return;
        if (text.trimmed().isEmpty()) showAllRecords();
        else debounce_->start();  // restarts on each keystroke
    });
    connect(keywordEdit_, &QLineEdit::returnPressed, this, &SearchPanel::runSearch);
    connect(searchButton, &QPushButton::clicked, this, &SearchPanel::runSearch);
    connect(showAllButton, &QPushButton::clicked, this, [this] {
        keywordEdit_->clear();
        showAllRecords();
    });
    connect(sortButton, &QPushButton::clicked, this, &SearchPanel::runSort);
    connect(compareButton, &QPushButton::clicked, this, &SearchPanel::compareAlgorithms);
    connect(sortAlgoCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SearchPanel::updateSortAdvice);
    connect(sortFieldCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SearchPanel::updateSortAdvice);
    connect(fieldCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this] {
        if (!keywordEdit_->text().trimmed().isEmpty()) runLiveSearch();
    });
    connect(searchAlgoCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this] {
        if (!keywordEdit_->text().trimmed().isEmpty()) runLiveSearch();
    });
}

void SearchPanel::focusSearch() {
    keywordEdit_->setFocus();
    keywordEdit_->selectAll();
}

void SearchPanel::refresh() {
    updateStateLabel();
    if (keywordEdit_->text().trimmed().isEmpty()) showAllRecords();
    else search(false);
}

void SearchPanel::updateStateLabel() {
    const SortField sorted = manager_.database().sortedBy();
    QString text = "Current order: <b>" + q(model::sortFieldToString(sorted)) + "</b>";
    if (sorted == SortField::Id) text += "  -  binary search is available for Patient ID";
    else if (sorted == SortField::Name) text += "  -  binary search is available for Patient Name (prefix)";
    else text += "  -  data not sorted by a searchable key, so Auto will use linear/recursive search";
    stateLabel_->setText(text);
}

void SearchPanel::showAllRecords() {
    std::vector<int> all;
    for (int i = 0; i < static_cast<int>(manager_.database().size()); ++i) all.push_back(i);
    showRows(all);
    resultCount_->setText(QString("Showing all %1 patients in the current order").arg(all.size()));
}

void SearchPanel::showRows(const std::vector<int>& indices) {
    const auto& patients = manager_.database().patients();
    results_->setRowCount(static_cast<int>(indices.size()));
    for (int r = 0; r < static_cast<int>(indices.size()); ++r) fillPatientRow(results_, r, *patients[indices[r]]);
}

void SearchPanel::runSearch() { search(true); }

void SearchPanel::runLiveSearch() { search(false); }

void SearchPanel::search(bool interactive) {
    const auto field = static_cast<SearchField>(fieldCombo_->currentData().toInt());
    const auto algo = static_cast<SearchAlgorithm>(searchAlgoCombo_->currentData().toInt());
    const std::string keyword = s(keywordEdit_->text());

    try {
        const controller::SearchResult r = manager_.search(field, algo, keyword);
        showRows(r.indices);
        resultCount_->setText(QString("%1 match(es) for \"%2\" in %3")
                                  .arg(r.indices.size())
                                  .arg(keywordEdit_->text().trimmed(), q(controller::SearchEngine::fieldName(field))));
        info_->setHtml(QString("<b>Search:</b> %1<br><b>Comparisons:</b> %2 &nbsp; <b>Time:</b> %3 &micro;s"
                               "<br><b>Why:</b> %4")
                           .arg(q(r.algorithmUsed))
                           .arg(r.comparisons)
                           .arg(r.microseconds, 0, 'f', 1)
                           .arg(q(r.reason).toHtmlEscaped()));
        if (interactive && r.indices.empty())
            emit statusMessage("No patients matched \"" + keywordEdit_->text().trimmed() + "\".");
    } catch (const utils::ValidationException& e) {
        // Invalid search input: dialog on explicit search, inline note while typing.
        if (interactive) QMessageBox::warning(this, "Invalid Search", "Error: " + q(e.what()));
        info_->setHtml("<span style='color:#D93F45'><b>Search not run:</b> " + q(e.what()).toHtmlEscaped() + "</span>");
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Search Error", q(e.what()));
    }
}

void SearchPanel::updateSortAdvice() {
    const auto algo = static_cast<SortAlgorithm>(sortAlgoCombo_->currentData().toInt());
    const auto field = static_cast<SortField>(sortFieldCombo_->currentData().toInt());
    const std::size_t n = manager_.database().size();
    const bool same = manager_.database().sortedBy() == field;
    const SortAlgorithm suggested = SortEngine::suggest(n, same);
    info_->setHtml(QString("<b>%1</b> - %2<br>%3<br><i>Suggested for %4 records%5: %6</i>")
                       .arg(q(SortEngine::algorithmName(algo)), q(SortEngine::complexity(algo)),
                            q(SortEngine::recommendation(algo, n, same)).toHtmlEscaped())
                       .arg(n)
                       .arg(same ? " already in this order" : "")
                       .arg(q(SortEngine::algorithmName(suggested))));
}

void SearchPanel::runSort() {
    const auto algo = static_cast<SortAlgorithm>(sortAlgoCombo_->currentData().toInt());
    const auto field = static_cast<SortField>(sortFieldCombo_->currentData().toInt());
    const bool ascending = orderCombo_->currentIndex() == 0;
    const bool wasSorted = manager_.database().sortedBy() == field;
    const std::size_t n = manager_.database().size();

    controller::SortStats stats;
    if (!runGuarded(this, [&] { stats = manager_.sort(algo, field, ascending); })) return;

    keywordEdit_->blockSignals(true);
    keywordEdit_->clear();
    keywordEdit_->blockSignals(false);
    showAllRecords();
    updateStateLabel();
    info_->setHtml(QString("<b>Sorted %1 records by %2 (%3) using %4.</b><br>"
                           "<b>Comparisons:</b> %5 &nbsp; <b>Moves/swaps:</b> %6 &nbsp; <b>Time:</b> %7 &micro;s"
                           " &nbsp; <b>Complexity:</b> %8<br><b>Why this algorithm:</b> %9")
                       .arg(n)
                       .arg(q(model::sortFieldToString(field)), ascending ? "ascending" : "descending",
                            q(SortEngine::algorithmName(algo)))
                       .arg(stats.comparisons)
                       .arg(stats.moves)
                       .arg(stats.microseconds, 0, 'f', 1)
                       .arg(q(SortEngine::complexity(algo)), q(SortEngine::recommendation(algo, n, wasSorted)).toHtmlEscaped()));
    emit dataChanged();
    emit statusMessage(QString("Sorted by %1 using %2.").arg(q(model::sortFieldToString(field)),
                                                             q(SortEngine::algorithmName(algo))));
}

void SearchPanel::compareAlgorithms() {
    const auto field = static_cast<SortField>(sortFieldCombo_->currentData().toInt());
    const bool ascending = orderCombo_->currentIndex() == 0;
    QString rows;
    for (auto algo : {SortAlgorithm::Selection, SortAlgorithm::Insertion, SortAlgorithm::Merge, SortAlgorithm::Quick}) {
        // Work on a deep copy so the real data is untouched.
        SortEngine::PatientList copy;
        for (const auto& p : manager_.database().patients()) copy.push_back(p->clone());
        const auto st = SortEngine::sort(copy, algo, field, ascending);
        rows += QString("<tr><td>%1</td><td align='right'>%2</td><td align='right'>%3</td><td align='right'>%4</td></tr>")
                    .arg(q(SortEngine::algorithmName(algo)))
                    .arg(st.comparisons)
                    .arg(st.moves)
                    .arg(st.microseconds, 0, 'f', 1);
    }
    info_->setHtml(QString("<b>Comparison on the current %1 records, sorting by %2:</b>"
                           "<table cellspacing='0' cellpadding='3'><tr><th align='left'>Algorithm</th>"
                           "<th>Comparisons</th><th>Moves</th><th>Time (&micro;s)</th></tr>%3</table>")
                       .arg(manager_.database().size())
                       .arg(q(model::sortFieldToString(field)), rows));
}

} // namespace scapms::gui
