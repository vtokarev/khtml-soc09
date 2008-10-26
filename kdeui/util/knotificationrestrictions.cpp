/* This file is part of the KDE libraries
    Copyright (C) 2006 Aaron Seigo <aseigo@kde.org>

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

#include "knotificationrestrictions.h"

#include <kdebug.h>

#include <config.h>

#ifdef HAVE_XTEST
#include <QTimer>
#include <QX11Info>

#include <X11/keysym.h>
#include <X11/extensions/XTest.h>
#endif // HAVE_XTEST

class KNotificationRestrictions::Private
{
    public:
        Private( KNotificationRestrictions* qq, Services c )
            : q( qq ),
              control(c)
#ifdef HAVE_XTEST
             ,screensaverTimer(0),
              haveXTest(0),
              XTestKeyCode(0)
#endif // HAVE_XTEST
        {
        }

        void screensaverFakeKeyEvent();
        void startScreenSaverPrevention();
        void stopScreenSaverPrevention();

        KNotificationRestrictions* q;
        Services control;
#ifdef HAVE_XTEST
        QTimer* screensaverTimer;
        int haveXTest;
        int XTestKeyCode;
#endif // HAVE_XTEST
};

KNotificationRestrictions::KNotificationRestrictions( Services control,
                                                      QObject* parent )
    : QObject(parent),
      d( new Private( this, control ) )
{
    if (d->control & ScreenSaver) {
        d->startScreenSaverPrevention();
    }
}

KNotificationRestrictions::~KNotificationRestrictions()
{
    if (d->control & ScreenSaver) {
        d->stopScreenSaverPrevention();
    }

    delete d;
}

void KNotificationRestrictions::Private::screensaverFakeKeyEvent()
{
    kDebug(297);
#ifdef HAVE_XTEST
    kDebug(297) << "---- using XTestFakeKeyEvent";
    Display* display = QX11Info::display();
    XTestFakeKeyEvent(display, XTestKeyCode, true, CurrentTime);
    XTestFakeKeyEvent(display, XTestKeyCode, false, CurrentTime);
    XSync(display, false);
#endif // HAVE_XTEST
}

void KNotificationRestrictions::Private::startScreenSaverPrevention()
{
    kDebug(297);
#ifdef HAVE_XTEST
    if ( !haveXTest ) {
        int a,b,c,e;
        haveXTest = XTestQueryExtension(QX11Info::display(), &a, &b, &c, &e);

        if ( !haveXTest ) {
            kDebug(297) << "--- No XTEST!";
            return;
        }
    }

    if ( !XTestKeyCode ) {
        XTestKeyCode = XKeysymToKeycode(QX11Info::display(), XK_Shift_L);

        if ( !XTestKeyCode ) {
            kDebug(297) << "--- No XKeyCode for XK_Shift_L!";
            return;
        }
    }

    if ( !screensaverTimer ) {
        screensaverTimer = new QTimer( q );
        connect( screensaverTimer, SIGNAL(timeout()),
                 q, SLOT(screensaverFakeKeyEvent()) );
    }

    kDebug(297) << "---- using XTest";
    // send a fake event right away in case this got started after a period of
    // innactivity leading to the screensaver set to activate in <55s
    screensaverFakeKeyEvent();
    screensaverTimer->start( 55000 );
#endif // HAVE_XTEST
}

void KNotificationRestrictions::Private::stopScreenSaverPrevention()
{
#ifdef HAVE_XTEST
    delete screensaverTimer;
    screensaverTimer = 0;
#endif // HAVE_XTEST
}
#include "knotificationrestrictions.moc"
