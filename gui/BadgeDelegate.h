#pragma once

#include <QColor>
#include <QHash>
#include <QStyledItemDelegate>

namespace scapms::gui {

// Custom cell rendering: draws the cell text as a coloured rounded "pill".
// Used for patient priority (High Priority / Normal / Scheduled) and
// appointment status (Pending / Completed / Cancelled).
class BadgeDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit BadgeDelegate(QHash<QString, QColor> colours, QObject* parent = nullptr);

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    static BadgeDelegate* forPriority(QObject* parent);
    static BadgeDelegate* forStatus(QObject* parent);

private:
    QHash<QString, QColor> colours_;
};

} // namespace scapms::gui
