/* This file is part of the KDE libraries
   Copyright 2009 by Marco Martin <notmart@gmail.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License (LGPL) as published by the Free Software Foundation;
   either version 2 of the License, or (at your option) any later
   version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "knotificationitemtest.h"

#include "../knotificationitem.h"
#include <QDateTime>
#include <QtGui/QLabel>
#include <QMovie>

#include <kapplication.h>
#include <kcmdlineargs.h>
#include <kdebug.h>
#include <kaboutdata.h>
#include <kmenu.h>
#include <kicon.h>

using namespace Experimental;

KNotificationItemTest::KNotificationItemTest(QObject *parent, KNotificationItem *tray)
  : QObject(parent)
{
    KMenu *menu = tray->contextMenu();
    m_tray = tray;

    QAction *needsAttention = new QAction("Set needs attention", menu);
    QAction *active = new QAction("Set active", menu);
    QAction *passive = new QAction("Set passive", menu);

    menu->addAction(needsAttention);
    menu->addAction(active);
    menu->addAction(passive);

    connect(needsAttention, SIGNAL(triggered()), this, SLOT(setNeedsAttention()));
    connect(active, SIGNAL(triggered()), this, SLOT(setActive()));
    connect(passive, SIGNAL(triggered()), this, SLOT(setPassive()));
}

void KNotificationItemTest::setNeedsAttention()
{
    kDebug()<<"Asking for attention";
    m_tray->setStatus(KNotificationItem::NeedsAttention);
}

void KNotificationItemTest::setActive()
{
    kDebug()<<"Systray icon in active state";
    m_tray->setStatus(KNotificationItem::Active);
}

void KNotificationItemTest::setPassive()
{
    kDebug()<<"Systray icon in passive state";
    m_tray->setStatus(KNotificationItem::Passive);
}

int main(int argc, char **argv)
{
    KAboutData aboutData( "KNotificationItemtest", 0 , ki18n("KNotificationItemtest"), "1.0" );
    KCmdLineArgs::init(argc, argv, &aboutData);
    KApplication app;
    QLabel *l = new QLabel("System Tray Main Window", 0L);
    KNotificationItem *tray = new KNotificationItem(l);

    KNotificationItemTest *trayTest = new KNotificationItemTest(0, tray);


    tray->setTitle("DBus System tray test");
    tray->setIconByName("konqueror");
    //tray->setImage(KIcon("konqueror"));
    tray->setAttentionIconByName("kmail");
    tray->setOverlayIconByName("emblem-important");
    //tray->setAttentionMovie(KIconLoader::global()->loadMovie( QLatin1String( "newmessage" ), KIconLoader::Panel ));

    tray->setToolTipIconByName("konqueror");
    tray->setToolTipTitle("DBus System tray test");
    tray->setToolTipSubTitle("This is a test of the new systemtray specification");

    //tray->setToolTip("konqueror", "DBus System tray test", "This is a test of the new systemtray specification");

    tray->showMessage("message test", "Test of the new systemtray notifications wrapper", "konqueror", 3000);
    //tray->setStandardActionsEnabled(false);


    return app.exec();
}

#include <knotificationitemtest.moc>
