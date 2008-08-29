/* This file is part of the KDE libraries Copyright (C) 1998 Mark Donohoe <donohoe@kde.org>
    Copyright (C) 1997 Nicolas Hadacek <hadacek@kde.org>
    Copyright (C) 1998 Matthias Ettrich <ettrich@kde.org>
    Copyright (C) 2001 Ellis Whitehead <ellis@kde.org>
    Copyright (C) 2006 Hamish Rodda <rodda@kde.org>
    Copyright (C) 2007 Roberto Raggi <roberto@kdevelop.org>
    Copyright (C) 2007 Andreas Hartmetz <ahartmetz@gmail.com>
    Copyright (C) 2008 Michael Jansen <kde@michael-jansen.biz>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/



#include "kshortcutsdialog_p.h"

#include <QApplication>
#include <QHeaderView>
#include <QLabel>
#include <QPainter>

#include "kaction.h"

#include "kdebug.h"




KShortcutsEditorDelegate::KShortcutsEditorDelegate(QTreeWidget *parent, bool allowLetterShortcuts)
 : KExtendableItemDelegate(parent),
   m_allowLetterShortcuts(allowLetterShortcuts),
   m_editor(0)
{
    Q_ASSERT(qobject_cast<QAbstractItemView *>(parent));

    QPixmap pixmap( 16, 16 );
    pixmap.fill( QColor( Qt::transparent ) );
    QPainter p( &pixmap );
    QStyleOption option;
    option.rect = pixmap.rect();

    bool isRtl = QApplication::isRightToLeft();
    QApplication::style()->drawPrimitive( isRtl ? QStyle::PE_IndicatorArrowLeft : QStyle::PE_IndicatorArrowRight, &option, &p );
    setExtendPixmap( pixmap );

    pixmap.fill( QColor( Qt::transparent ) );
    QApplication::style()->drawPrimitive( QStyle::PE_IndicatorArrowDown, &option, &p );
    setContractPixmap( pixmap );

    // Listen to activiation signals
    // connect(parent, SIGNAL(activated(QModelIndex)), this, SLOT(itemActivated(QModelIndex)));
    connect(parent, SIGNAL(clicked(QModelIndex)), this, SLOT(itemActivated(QModelIndex)));

    // Listen to collapse signals
    connect(parent, SIGNAL(collapsed(QModelIndex)), this, SLOT(itemCollapsed(QModelIndex)));
}


QSize KShortcutsEditorDelegate::sizeHint(const QStyleOptionViewItem &option,
                                         const QModelIndex &index) const
{
    QSize ret(KExtendableItemDelegate::sizeHint(option, index));
    ret.rheight() += 4;
    return ret;
}


//slot
void KShortcutsEditorDelegate::itemActivated(QModelIndex index)
{
    //As per our constructor our parent *is* a QTreeWidget
    QTreeWidget *view = static_cast<QTreeWidget *>(parent());

    KShortcutsEditorItem *item = KShortcutsEditorPrivate::itemFromIndex(view, index);
    if (!item) {
        //that probably was a non-leaf (type() !=ActionItem) item
        return;
    }

    int column = index.column();
    if (column == Name) {
        // If user click in the name column activate the (Global|Local)Primary
        // column if possible.
        if (!view->header()->isSectionHidden(LocalPrimary)) {
            column = LocalPrimary;
        } else if (!view->header()->isSectionHidden(GlobalPrimary)) {
            column = GlobalPrimary;
        } else {
            // do nothing.
        }
        index = index.sibling(index.row(), column);
        view->selectionModel()->select(index, QItemSelectionModel::SelectCurrent);
    }

    // Check if the models wants us to edit the item at index
    if (!index.data(ShowExtensionIndicatorRole).value<bool>()) {
        return;
    }

    if (!isExtended(index)) {
        //we only want maximum ONE extender open at any time.
        if (m_editingIndex.isValid()) {
            QModelIndex idx = index.sibling(m_editingIndex.row(), Name);
            KShortcutsEditorItem *oldItem = KShortcutsEditorPrivate::itemFromIndex(view, idx);
            Q_ASSERT(oldItem); //here we really expect nothing but a real KShortcutsEditorItem

            oldItem->setNameBold(false);
            contractItem(m_editingIndex);
        }

        m_editingIndex = index;
        QWidget *viewport = static_cast<QAbstractItemView*>(parent())->viewport();

        if (column >= LocalPrimary && column <= GlobalAlternate) {
            ShortcutEditWidget *editor = new ShortcutEditWidget(viewport,
                      index.data(DefaultShortcutRole).value<QKeySequence>(),
                      index.data(ShortcutRole).value<QKeySequence>(),
                      m_allowLetterShortcuts);
            if (column==GlobalPrimary) {
                QObject *action = index.data(ObjectRole).value<QObject*>();
                connect(
                    action, SIGNAL(globalShortcutChanged(QKeySequence)),
                    editor, SLOT(setKeySequence(QKeySequence)));
                }


            m_editor = editor;
            // For global shortcuts check against the kde standard shortcuts
            if (column == GlobalPrimary || column == GlobalAlternate) {
                editor->setCheckForConflictsAgainst(
                    // Don't know why that is necessary but it doesn't compile
                    // without.
                    KKeySequenceWidget::ShortcutTypes(
                        KKeySequenceWidget::LocalShortcuts
                            | KKeySequenceWidget::GlobalShortcuts
                            | KKeySequenceWidget::StandardShortcuts ));
            }

            editor->setCheckActionCollections(m_checkActionCollections);

            connect(m_editor, SIGNAL(keySequenceChanged(const QKeySequence &)),
                    this, SLOT(keySequenceChanged(const QKeySequence &)));

        } else if (column == RockerGesture) {
            m_editor = new QLabel("A lame placeholder", viewport);

        } else if (column == ShapeGesture) {
            m_editor = new QLabel("<i>A towel</i>", viewport);

        } else
            return;

        m_editor->installEventFilter(this);
        item->setNameBold(true);
        extendItem(m_editor, index);

    } else {
        //the item is extended, and clicking on it again closes it
        item->setNameBold(false);
        contractItem(index);
        view->selectionModel()->select(index, QItemSelectionModel::Clear);
        m_editingIndex = QModelIndex();
        m_editor = 0;
    }
}


//slot
void KShortcutsEditorDelegate::itemCollapsed(QModelIndex index)
{
    if (!m_editingIndex.isValid())
        return;

    const QAbstractItemModel *model = index.model();
    for (int row=0; row<model->rowCount(index);++row) {
        QModelIndex rowIndex = model->index(row,0,index);

        for (int col=0; col<index.model()->columnCount(index);++col) {
            QModelIndex colIndex = model->index(row,col,index);

            if (colIndex == m_editingIndex) {
                itemActivated(m_editingIndex); //this will *close* the item's editor because it's already open
            }

        }

    }

}


//slot
void KShortcutsEditorDelegate::hiddenBySearchLine(QTreeWidgetItem *item, bool hidden)
{
    if (!hidden || !item) {
        return;
    }
    QTreeWidget *view = static_cast<QTreeWidget *>(parent());
    QTreeWidgetItem *editingItem = KShortcutsEditorPrivate::itemFromIndex(view, m_editingIndex);
    if (editingItem == item) {
        itemActivated(m_editingIndex); //this will *close* the item's editor because it's already open
    }
}


//Prevent clicks in the empty part of the editor widget from closing the editor
//because they would propagate to the itemview and be interpreted as a click in
//an item's rect. That in turn would lead to an itemActivated() call, closing
//the current editor.
bool KShortcutsEditorDelegate::eventFilter(QObject *o, QEvent *e)
{
    if (o != m_editor)
        return false;

    switch (e->type()) {
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
        return true;
    default:
        return false;
    }
}


//slot
void KShortcutsEditorDelegate::keySequenceChanged(const QKeySequence &seq)
{
    QVariant ret = QVariant::fromValue(seq);
    emit shortcutChanged(ret, m_editingIndex);
}


void KShortcutsEditorDelegate::setCheckActionCollections(
    const QList<KActionCollection*> checkActionCollections )
{
    m_checkActionCollections = checkActionCollections;
}

//slot
void KShortcutsEditorDelegate::shapeGestureChanged(const KShapeGesture &gest)
{
    //this is somewhat verbose because the gesture types are not "built in" to QVariant
    QVariant ret = QVariant::fromValue(gest);
    emit shortcutChanged(ret, m_editingIndex);
}


//slot
void KShortcutsEditorDelegate::rockerGestureChanged(const KRockerGesture &gest)
{
    QVariant ret = QVariant::fromValue(gest);
    emit shortcutChanged(ret, m_editingIndex);
}

