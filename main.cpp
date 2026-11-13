#include "gui/MainWindow.h"

#include <QApplication>
#include <QMessageBox>
#include <QPalette>
#include <QStyleFactory>

#include <exception>

namespace {

// Fixed light theme so category colours stay readable on every OS theme.
void applyTheme(QApplication& app) {
    app.setStyle(QStyleFactory::create("Fusion"));
    QPalette p;
    p.setColor(QPalette::Window, QColor("#F4F5F8"));
    p.setColor(QPalette::WindowText, QColor("#1E2030"));
    p.setColor(QPalette::Base, QColor("#FFFFFF"));
    p.setColor(QPalette::AlternateBase, QColor("#F7F8FA"));
    p.setColor(QPalette::Text, QColor("#1E2030"));
    p.setColor(QPalette::Button, QColor("#FFFFFF"));
    p.setColor(QPalette::ButtonText, QColor("#1E2030"));
    p.setColor(QPalette::Highlight, QColor("#C9D4F7"));
    p.setColor(QPalette::HighlightedText, QColor("#111322"));
    p.setColor(QPalette::ToolTipBase, QColor("#1E2030"));
    p.setColor(QPalette::ToolTipText, QColor("#FFFFFF"));
    p.setColor(QPalette::PlaceholderText, QColor("#9A9CA5"));
    app.setPalette(p);

    app.setStyleSheet(R"(
        QGroupBox { font-weight: 600; border: 1px solid #DDE0E8; border-radius: 8px; margin-top: 14px;
                    padding: 10px 8px 8px 8px; background: #FFFFFF; }
        QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 4px; color: #3A3D52; }
        QPushButton { padding: 6px 14px; border: 1px solid #C9CCD6; border-radius: 6px; background: #FFFFFF; }
        QPushButton:hover { background: #EEF1F8; }
        QPushButton:disabled { color: #A9ABB4; background: #F4F5F8; }
        QPushButton#primaryButton { background: #3A5CCC; color: white; border-color: #3A5CCC; font-weight: 600; }
        QPushButton#primaryButton:hover { background: #2F4DB3; }
        QPushButton#primaryButton:disabled { background: #A9B6E3; border-color: #A9B6E3; }
        QPushButton#dangerButton { color: #C2333A; border-color: #E6B3B6; }
        QPushButton#dangerButton:hover { background: #FDEDEE; }
        QLineEdit, QComboBox, QDateEdit, QTimeEdit { padding: 4px 6px; border: 1px solid #C9CCD6;
                    border-radius: 5px; background: #FFFFFF; min-height: 20px; }
        QLineEdit:focus, QComboBox:focus, QDateEdit:focus, QTimeEdit:focus { border-color: #3A5CCC; }
        QTableWidget, QListWidget, QTextBrowser { border: 1px solid #DDE0E8; border-radius: 6px; background: #FFFFFF; }
        QHeaderView::section { background: #F0F2F7; border: none; border-bottom: 1px solid #DDE0E8;
                    padding: 6px; font-weight: 600; color: #3A3D52; }
        QTabBar::tab { padding: 8px 16px; }
        QTabBar::tab:selected { color: #3A5CCC; font-weight: 600; }
        #statCard { background: #FFFFFF; border: 1px solid #DDE0E8; border-radius: 8px; }
        #cardTitle { color: #6B6E7B; font-size: 12px; }
        #cardValue { font-size: 30px; font-weight: 700; }
        #sectionTitle { font-size: 16px; font-weight: 700; color: #1E2030; }
        #mutedLabel { color: #6B6E7B; }
        #hintLabel { color: #6B6E7B; font-size: 12px; }
        #stateLabel { color: #3A3D52; padding: 2px; }
        #servingLabel { font-size: 15px; font-weight: 700; color: #3A5CCC; }
        #reportSummary { color: #3A3D52; padding: 2px 0 6px 0; }
        #statusPill { color: #4A4D5E; padding: 0 8px; border-left: 1px solid #DDE0E8; }
    )");
}

} // namespace

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName("SCAPMS");
    QApplication::setOrganizationName("COS202");
    applyTheme(app);

    try {
        scapms::gui::MainWindow window;
        window.resize(1320, 820);
        window.show();
        window.loadStartupData();
        return app.exec();
    } catch (const std::exception& e) {
        // Last line of defence: report instead of crashing silently.
        QMessageBox::critical(nullptr, "Fatal Error", QString("SCAPMS must close:\n%1").arg(e.what()));
        return 1;
    }
}
