/* This file is part of the KDE libraries
   Copyright (C) 2009 Dario Freddi <drf at kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef XSYNCBASEDPOLLER_H
#define XSYNCBASEDPOLLER_H

#include "abstractsystempoller.h"

#define HAVE_XSYNC 1 // Hack until the library will be moved away. Too lazy to implement something temporary

#include <KDebug>
#include <QWidget>
#include <KApplication>

#ifdef HAVE_XSYNC
#include <X11/Xlib.h>
#include <X11/extensions/sync.h>
#endif

class XSyncBasedPoller : public AbstractSystemPoller
{
    Q_OBJECT

public:
    static XSyncBasedPoller *instance();

    virtual ~XSyncBasedPoller();

    bool isAvailable();
    bool setUpPoller();
    void unloadPoller();

protected:
    bool x11Event(XEvent *event);
    XSyncBasedPoller(QWidget *parent = 0);

public slots:
    void addTimeout(int nextTimeout);
    void removeTimeout(int nextTimeout);
    QList<int> timeouts() const;
    int forcePollRequest();
    void catchIdleEvent();
    void stopCatchingIdleEvents();
    void simulateUserActivity();

private slots:
    int poll();
    void reloadAlarms();

signals:
    void resumingFromIdle();
    void timeoutReached(int msec);

#ifdef HAVE_XSYNC
private:
    void setAlarm(Display *dpy, XSyncAlarm *alarm, XSyncCounter counter,
                  XSyncTestType test, XSyncValue value);
#endif

private:
#ifdef HAVE_XSYNC
    Display * m_display;
    int                 m_sync_event, m_sync_error;
    XSyncSystemCounter  *m_counters;
    XSyncCounter        m_idleCounter;
    QHash<int, XSyncAlarm>   m_timeoutAlarm;
    XSyncAlarm          m_resetAlarm;
#endif
    QWidget * m_filterWidget;
    bool m_available;
};

#endif /* XSYNCBASEDPOLLER_H */


