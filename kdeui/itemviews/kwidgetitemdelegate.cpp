/**
  * This file is part of the KDE project
  * Copyright (C) 2007-2008 Rafael Fernández López <ereslibre@kde.org>
  * Copyright (C) 2008 Kevin Ottens <ervin@kde.org>
  *
  * This library is free software; you can redistribute it and/or
  * modify it under the terms of the GNU Library General Public
  * License as published by the Free Software Foundation; either
  * version 2 of the License, or (at your option) any later version.
  *
  * This library is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  * Library General Public License for more details.
  *
  * You should have received a copy of the GNU Library General Public License
  * along with this library; see the file COPYING.LIB.  If not, write to
  * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  * Boston, MA 02110-1301, USA.
  */

#include "kwidgetitemdelegate.h"
#include "kwidgetitemdelegate_p.h"

#include <QIcon>
#include <QSize>
#include <QStyle>
#include <QEvent>
#include <QHoverEvent>
#include <QFocusEvent>
#include <QCursor>
#include <QBitmap>
#include <QLayout>
#include <QPainter>
#include <QScrollBar>
#include <QKeyEvent>
#include <QStyleOption>
#include <QPaintEngine>
#include <QCoreApplication>
#include <QAbstractItemView>

#include "kwidgetitemdelegatepool_p.h"

Q_DECLARE_METATYPE(QList<QEvent::Type>)

/**
  Private class that helps to provide binary compatibility between releases.
  @internal
*/
//@cond PRIVATE
KWidgetItemDelegatePrivate::KWidgetItemDelegatePrivate(KWidgetItemDelegate *q, QObject *parent)
    : QObject(parent)
    , itemView(0)
    , hoveredIndex(QPersistentModelIndex())
    , lastHoveredIndex(QPersistentModelIndex())
    , focusedIndex(QPersistentModelIndex())
    , currentIndex(QPersistentModelIndex())
    , selectionModel(0)
    , widgetPool(new KWidgetItemDelegatePool(q))
    , q(q)
{
}

KWidgetItemDelegatePrivate::~KWidgetItemDelegatePrivate()
{
    delete widgetPool;
}

// When receiving move events on the viewport we need to check if we should
// post events to certain widgets like "enter", "leave"...
// Note: mouseEvent can be 0
void KWidgetItemDelegatePrivate::analyzeInternalMouseEvents(const QStyleOptionViewItem &option,
                                                            QMouseEvent *mouseEvent)
{
}

QPoint KWidgetItemDelegatePrivate::mappedPointForWidget(QWidget *widget,
                                                        const QPersistentModelIndex &index,
                                                        const QPoint &pos) const
{
    // Map the event point relative to the widget
    QStyleOptionViewItem option;
    option.rect = itemView->visualRect(index);
    QPoint widgetPos = widget->pos();
    widgetPos.setX(widgetPos.x() + option.rect.left());
    widgetPos.setY(widgetPos.y() + option.rect.top());

    return pos - widgetPos;
}

void KWidgetItemDelegatePrivate::slotCurrentChanged(const QModelIndex &currentIndex,
                                                    const QModelIndex &previousIndex)
{
    Q_UNUSED(previousIndex);

    this->currentIndex = currentIndex;
}

void KWidgetItemDelegatePrivate::slotSelectionModelDestroyed()
{
    selectionModel = 0;
}
//@endcond

KWidgetItemDelegate::KWidgetItemDelegate(QAbstractItemView *itemView, QObject *parent)
    : QAbstractItemDelegate(parent)
    , d(new KWidgetItemDelegatePrivate(this))
{
    Q_ASSERT(itemView);

    itemView->setMouseTracking(true);
    itemView->viewport()->setAttribute(Qt::WA_Hover);

    d->itemView = itemView;

    itemView->viewport()->installEventFilter(d); // mouse events
    itemView->installEventFilter(d);             // keyboard events

    if (itemView->selectionModel()) {
        d->selectionModel = itemView->selectionModel();

        connect(itemView->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
                d, SLOT(slotCurrentChanged(QModelIndex,QModelIndex)));
    }
}

KWidgetItemDelegate::~KWidgetItemDelegate()
{
    delete d;
}

QAbstractItemView *KWidgetItemDelegate::itemView() const
{
    return d->itemView;
}

QPersistentModelIndex KWidgetItemDelegate::focusedIndex() const
{
    return d->focusedIndex;
}

//@cond PRIVATE
QRect KWidgetItemDelegatePrivate::widgetRect(QWidget *widget,
                                             const QStyleOptionViewItem &option,
                                             const QPersistentModelIndex &index) const
{
    Q_UNUSED(index);
    QRect retRect = QRect(widget->pos(), widget->size());
    retRect.translate(option.rect.topLeft());
    return retRect;
}
//@endcond

void KWidgetItemDelegate::paintWidgets(QPainter *painter, const QStyleOptionViewItem &option,
                                       const QPersistentModelIndex &index) const
{
}

//@cond PRIVATE
bool KWidgetItemDelegatePrivate::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::Destroy) {
        return false;
    }

    Q_UNUSED(watched);
    Q_ASSERT(itemView);

    if (selectionModel != itemView->selectionModel()) {
        if (selectionModel) {
            disconnect(selectionModel, SIGNAL(currentChanged(QModelIndex,QModelIndex)),
                       this, SLOT(slotCurrentChanged(QModelIndex,QModelIndex)));
        }

        selectionModel = itemView->selectionModel();

        if (selectionModel) {
            connect(selectionModel, SIGNAL(currentChanged(QModelIndex,QModelIndex)),
                    this, SLOT(slotCurrentChanged(QModelIndex,QModelIndex)));
            connect(selectionModel, SIGNAL(destroyed(QObject*)),
                    this, SLOT(slotSelectionModelDestroyed()));
        }
    }

    bool filterEvent = false;
    bool eventReply = false;

    switch (event->type()) {
        case QEvent::Paint:
        case QEvent::Timer:
        case QEvent::UpdateRequest:
        case QEvent::Destroy:
        case QEvent::MetaCall:
            return false;
        case QEvent::Leave:
        case QEvent::Enter:
        case QEvent::MouseMove: {
                QStyleOptionViewItem styleOptionViewItem;
                styleOptionViewItem.initFrom(itemView);
                styleOptionViewItem.rect = itemView->visualRect(hoveredIndex);

                QMouseEvent *mouseEvent = dynamic_cast<QMouseEvent*>(event);
                lastHoveredIndex = hoveredIndex;
                hoveredIndex = itemView->indexAt(itemView->viewport()->mapFromGlobal(QCursor::pos()));
            }
            break;
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease: {
                focusedIndex = hoveredIndex;

                QStyleOptionViewItem styleOptionViewItem;
                styleOptionViewItem.initFrom(itemView);
                styleOptionViewItem.rect = itemView->visualRect(focusedIndex);
                QList<QWidget*> widgetList = widgetPool->findWidgets(focusedIndex, styleOptionViewItem);
                foreach (QWidget *widget, widgetList) {
                    QPoint widgetPos = widget->pos();
                    QSize widgetSize = widget->size();
                    widget->move(widgetPos.x() + styleOptionViewItem.rect.left(), widgetPos.y() + styleOptionViewItem.rect.top());
                }

                QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

                if (event->type() == QEvent::MouseButtonPress) {
                    buttonPressedIndex = hoveredIndex;
                } else {
                    buttonPressedIndex = QModelIndex();
                }
            }
            break;
        default: {
                if (event->type() == QEvent::FocusOut) {
                    buttonPressedIndex = QModelIndex();
                }
            }
            break;
    }

    return filterEvent || eventReply || QObject::eventFilter(watched, event);
}
//@endcond

void KWidgetItemDelegate::setBlockedEventTypes(QWidget *widget, QList<QEvent::Type> types) const
{
    widget->setProperty("goya:blockedEventTypes", qVariantFromValue(types));
}

QList<QEvent::Type> KWidgetItemDelegate::blockedEventTypes(QWidget *widget) const
{
    return widget->property("goya:blockedEventTypes").value<QList<QEvent::Type> >();
}

#include "kwidgetitemdelegate_p.moc"
