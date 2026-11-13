#pragma once

#include "BasePanel.h"

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QTableWidget;
class QTextBrowser;
class QTimer;

namespace scapms::gui {

// Search & Sort tab: choose the search algorithm and the sorting algorithm.
class SearchPanel : public BasePanel {
    Q_OBJECT
public:
    explicit SearchPanel(controller::ClinicManager& manager, QWidget* parent = nullptr);

    void focusSearch();

public slots:
    void refresh() override;

private slots:
    void runSearch();          // explicit search: errors shown in a dialog
    void runLiveSearch();      // search-as-you-type: errors shown inline
    void runSort();
    void compareAlgorithms();
    void showAllRecords();
    void updateSortAdvice();

private:
    void search(bool interactive);
    void showRows(const std::vector<int>& indices);
    void updateStateLabel();

    QLineEdit* keywordEdit_;
    QComboBox* fieldCombo_;
    QComboBox* searchAlgoCombo_;
    QCheckBox* liveCheck_;
    QTimer* debounce_;

    QComboBox* sortAlgoCombo_;
    QComboBox* sortFieldCombo_;
    QComboBox* orderCombo_;

    QLabel* stateLabel_;
    QTextBrowser* info_;
    QTableWidget* results_;
    QLabel* resultCount_;
};

} // namespace scapms::gui
