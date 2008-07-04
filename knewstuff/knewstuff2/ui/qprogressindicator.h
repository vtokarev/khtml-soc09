/*
    This file is part of KNewStuff2.
    Copyright (c) 2007 Josef Spillner <spillner@kde.org>
    Copyright (c) 2008 Jeremy Whiting <jeremy@scitools.com>

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

#ifndef KNEWSTUFF2_UI_QPROGRESSINDICATOR_H
#define KNEWSTUFF2_UI_QPROGRESSINDICATOR_H

#include <QtGui/QFrame>
#include <QtCore/QHash>

#include <QtCore/QString>

class QVBoxLayout;
class QProgressBar;
class QPushButton;

/**
 * Embedded progress indicator for the download dialog.
 *
 * The indicator can display various asynchronous operations at once.
 * Each operation can also individually be cancelled.
 *
 * @internal
 */
class QProgressIndicator : public QFrame
{
    Q_OBJECT
public:
    QProgressIndicator(QWidget *parent);
    void addProgress(const QString & message, int percentage);
    void removeProgress(const QString & message);
public Q_SLOTS:
    void slotClicked();
signals:
    void signalJobCanceled(const QString & message);
private:
    void calculateAverage();

    QHash<QString, int> m_progress;
    QHash<QString, QProgressBar*> m_progresswidgets;
    QWidget *m_details;
    QVBoxLayout *m_detailsvbox;
    QProgressBar *m_pb;
    QPushButton *m_pbdetails;
};

#endif
