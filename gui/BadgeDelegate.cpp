#include "BadgeDelegate.h"

#include <QApplication>
#include <QPainter>
#include <QPainterPath>

#include <utility>

namespace scapms::gui {

BadgeDelegate::BadgeDelegate(QHash<QString, QColor> colours, QObject* parent)
    : QStyledItemDelegate(parent), colours_(std::move(colours)) {}

BadgeDelegate* BadgeDelegate::forPriority(QObject* parent) {
    return new BadgeDelegate({{"High Priority", QColor("#D93F45")},
                              {"Normal", QColor("#3A5CCC")},
                              {"Scheduled", QColor("#2A9461")}},
                             parent);
}

BadgeDelegate* BadgeDelegate::forStatus(QObject* parent) {
    return new BadgeDelegate({{"Pending", QColor("#C77C02")},
                              {"Completed", QColor("#2A9461")},
                              {"Cancelled", QColor("#80838D")}},
                             parent);
}

void BadgeDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    const QString text = index.data(Qt::DisplayRole).toString();
    const QColor colour = colours_.value(text, QColor("#80838D"));

    // Let the style draw background/selection, but not the text.
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    opt.text.clear();
    const QWidget* widget = option.widget;
    QStyle* style = widget ? widget->style() : QApplication::style();
    style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, widget);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    QFont font = option.font;
    font.setBold(true);
    font.setPointSizeF(font.pointSizeF() * 0.9);
    painter->setFont(font);

    const QFontMetrics fm(font);
    const int width = fm.horizontalAdvance(text) + 20;
    const int height = fm.height() + 6;
    const QRect pill(option.rect.left() + 6, option.rect.center().y() - height / 2 + 1, width, height);

    QPainterPath path;
    path.addRoundedRect(pill, height / 2.0, height / 2.0);
    painter->fillPath(path, colour);
    painter->setPen(Qt::white);
    painter->drawText(pill, Qt::AlignCenter, text);
    painter->restore();
}

QSize BadgeDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
    QSize base = QStyledItemDelegate::sizeHint(option, index);
    return {base.width() + 28, qMax(base.height(), 28)};
}

} // namespace scapms::gui
