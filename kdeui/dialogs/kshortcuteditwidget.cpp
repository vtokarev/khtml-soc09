/* This file is part of the KDE libraries Copyright (C) 1998 Mark Donohoe <donohoe@kde.org>
    Copyright (C) 1997 Nicolas Hadacek <hadacek@kde.org>
    Copyright (C) 1998 Matthias Ettrich <ettrich@kde.org>
    Copyright (C) 2001 Ellis Whitehead <ellis@kde.org>
    Copyright (C) 2006 Hamish Rodda <rodda@kde.org>
    Copyright (C) 2007 Roberto Raggi <roberto@kdevelop.org>
    Copyright (C) 2007 Andreas Hartmetz <ahartmetz@gmail.com>

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

#include <QPainter>
#include <QPen>
#include <QGridLayout>
#include <QRadioButton>
#include <QLabel>

#include "kkeysequencewidget.h"
#include "klocale.h"

void TabConnectedWidget::paintEvent(QPaintEvent *e)
{
    QWidget::paintEvent(e);
    QPainter p(this);
    QPen pen(QPalette().highlight().color());
    pen.setWidth(6);
    p.setPen(pen);
    p.drawLine(0, 0, width(), 0);
    p.drawLine(0, 0, 0, height());
}

ShortcutEditWidget::ShortcutEditWidget(QWidget *viewport, const QKeySequence &defaultSeq,
                                       const QKeySequence &activeSeq, bool allowLetterShortcuts)
 : TabConnectedWidget(viewport),
   m_defaultKeySequence(defaultSeq),
   m_isUpdating(false)
{
    QGridLayout *layout = new QGridLayout(this);

    m_defaultRadio = new QRadioButton(i18n("Default:"), this);
    QString defaultText = defaultSeq.toString(QKeySequence::NativeText);
    if (defaultText.isEmpty())
        defaultText = i18n("None");
    QLabel *defaultLabel = new QLabel(defaultText, this);

    m_customRadio = new QRadioButton(i18n("Custom:"), this);
    m_customEditor = new KKeySequenceWidget(this);
    m_customEditor->setModifierlessAllowed(allowLetterShortcuts);

    if (activeSeq == defaultSeq) {
        m_defaultRadio->setChecked(true);
    } else {
        m_customRadio->setChecked(true);
        m_customEditor->setKeySequence(activeSeq);
    }

    layout->addWidget(m_defaultRadio, 0, 0);
    layout->addWidget(defaultLabel, 0, 1);
    layout->addWidget(m_customRadio, 1, 0);
    layout->addWidget(m_customEditor, 1, 1);
    layout->setColumnStretch(2, 1);

    connect(m_defaultRadio, SIGNAL(toggled(bool)),
            this, SLOT(defaultToggled(bool)));
    connect(m_customEditor, SIGNAL(keySequenceChanged(const QKeySequence &)),
            this, SLOT(setCustom(const QKeySequence &)));
}


//slot
void ShortcutEditWidget::defaultToggled(bool checked)
{
    if (m_isUpdating)
        return;

    m_isUpdating = true;
    m_customEditor->clearKeySequence();
    if  (checked) {
        emit keySequenceChanged(m_defaultKeySequence);
    } else {
        //custom was checked
        emit keySequenceChanged(QKeySequence());
    }
    m_isUpdating = false;
}


void ShortcutEditWidget::setCheckActionCollections( 
        const QList<KActionCollection*> checkActionCollections)
{
    // We just forward them to out KKeySequenceWidget.
    m_customEditor->setCheckActionCollections(checkActionCollections);
}


//slot
void ShortcutEditWidget::setCustom(const QKeySequence &seq)
{
    if (m_isUpdating)
        return;

    m_isUpdating = true;
    if (seq != m_defaultKeySequence)
        m_customRadio->setChecked(true);
    else
        m_defaultRadio->setChecked(true);
    emit keySequenceChanged(seq);
    m_isUpdating = false;
}

