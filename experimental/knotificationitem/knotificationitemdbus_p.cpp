/***************************************************************************
 *   Copyright 2009 by Marco Martin <notmart@gmail.com>                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/

#include "knotificationitemdbus_p.h"
#include "knotificationitemprivate_p.h"
#include "knotificationitem.h"

#include <QDBusConnection>
#include <QPixmap>
#include <QImage>
#include <QApplication>
#include <QMovie>


#include <kdebug.h>
#include <ksystemtrayicon.h>
#include <kaboutdata.h>
#include <kicon.h>
#include <kmenu.h>
#include <kaction.h>
#include <kwindowinfo.h>
#include <kwindowsystem.h>
#include <kmessagebox.h>
#include <kactioncollection.h>

#include "notificationwatcher_interface.h"


#include "notificationitemadaptor.h"

using namespace Experimental;

// Marshall the ImageStruct data into a D-BUS argument
const QDBusArgument &operator<<(QDBusArgument &argument, const ExperimentalKDbusImageStruct &icon)
{
    argument.beginStructure();
    argument << icon.width;
    argument << icon.height;
    argument << icon.data;
    argument.endStructure();
    return argument;
}

// Retrieve the ImageStruct data from the D-BUS argument
const QDBusArgument &operator>>(const QDBusArgument &argument, ExperimentalKDbusImageStruct &icon)
{
    qint32 width;
    qint32 height;
    QByteArray data;

    argument.beginStructure();
    argument >> width;
    argument >> height;
    argument >> data;
    argument.endStructure();

    icon.width = width;
    icon.height = height;
    icon.data = data;

    return argument;
}


// Marshall the ImageVector data into a D-BUS argument
const QDBusArgument &operator<<(QDBusArgument &argument, const ExperimentalKDbusImageVector &iconVector)
{
    argument.beginArray(qMetaTypeId<ExperimentalKDbusImageStruct>());
    for (int i=0; i<iconVector.size(); ++i) {
        argument << iconVector[i]; 
    }
    argument.endArray();
    return argument;
}

// Retrieve the ImageVector data from the D-BUS argument
const QDBusArgument &operator>>(const QDBusArgument &argument, ExperimentalKDbusImageVector &iconVector)
{
    argument.beginArray();
    iconVector.clear();

    while ( !argument.atEnd() ) {
       ExperimentalKDbusImageStruct element;
       argument >> element;
       iconVector.append(element);
    }

    argument.endArray();


    return argument;
}

// Marshall the ToolTipStruct data into a D-BUS argument
const QDBusArgument &operator<<(QDBusArgument &argument, const ExperimentalKDbusToolTipStruct &toolTip)
{
    argument.beginStructure();
    argument << toolTip.icon;
    argument << toolTip.image;
    argument << toolTip.title;
    argument << toolTip.subTitle;
    argument.endStructure();
    return argument;
}

// Retrieve the ToolTipStruct data from the D-BUS argument
const QDBusArgument &operator>>(const QDBusArgument &argument, ExperimentalKDbusToolTipStruct &toolTip)
{
    QString icon;
    ExperimentalKDbusImageVector image;
    QString title;
    QString subTitle;

    argument.beginStructure();
    argument >> icon;
    argument >> image;
    argument >> title;
    argument >> subTitle;
    argument.endStructure();

    toolTip.icon = icon;
    toolTip.image = image;
    toolTip.title = title;
    toolTip.subTitle = subTitle;

    return argument;
}


int KNotificationItemDBus::s_serviceCount = 0;

KNotificationItemDBus::KNotificationItemDBus(KNotificationItem *parent)
  : QObject(parent),
    m_notificationItem(parent),
    m_service(QString("org.kde.NotificationItem-%1-%2")
                      .arg(QCoreApplication::applicationPid())
                      .arg(++s_serviceCount)),
    m_dbus(QDBusConnection::connectToBus(QDBusConnection::SessionBus, m_service))
{
   new NotificationItemAdaptor(this);
   kDebug() << "service is" << m_service;
   m_dbus.registerService(m_service);
   m_dbus.registerObject("/NotificationItem", this);
}

KNotificationItemDBus::~KNotificationItemDBus()
{
    m_dbus.unregisterService(m_service);
}

QString KNotificationItemDBus::service() const
{
    return m_service;
}

//DBUS slots

QString KNotificationItemDBus::Category() const
{
    return m_notificationItem->metaObject()->enumerator(m_notificationItem->metaObject()->indexOfEnumerator("ItemCategory")).valueToKey(m_notificationItem->category());
}

QString KNotificationItemDBus::Title() const
{
    return m_notificationItem->title();
}

QString KNotificationItemDBus::Status() const
 {
    return m_notificationItem->metaObject()->enumerator(m_notificationItem->metaObject()->indexOfEnumerator("ItemStatus")).valueToKey(m_notificationItem->status());
}

int KNotificationItemDBus::WindowId() const
{
    if (m_notificationItem->d->associatedWidget) {
        return m_notificationItem->d->associatedWidget->winId();
    } else {
        return 0;
    }
}


//Icon

QString KNotificationItemDBus::Icon() const
{
    return m_notificationItem->icon();
}

ExperimentalKDbusImageVector KNotificationItemDBus::Image() const
{
    return m_notificationItem->d->serializedIcon;
}

QString KNotificationItemDBus::OverlayIcon() const
{
    return m_notificationItem->overlayIcon();
}

ExperimentalKDbusImageVector KNotificationItemDBus::OverlayImage() const
{
    return m_notificationItem->d->serializedOverlayIcon;
}

//Requesting attention icon and movie

QString KNotificationItemDBus::AttentionIcon() const
{
    return m_notificationItem->attentionIcon();
}

ExperimentalKDbusImageVector KNotificationItemDBus::AttentionImage() const
{
    return m_notificationItem->d->serializedAttentionIcon;
}

ExperimentalKDbusImageVector KNotificationItemDBus::AttentionMovie() const
{
    return m_notificationItem->d->movieVector;
}


//ToolTip

ExperimentalKDbusToolTipStruct KNotificationItemDBus::ToolTip() const
{
    ExperimentalKDbusToolTipStruct toolTip;
    toolTip.icon = m_notificationItem->toolTipIcon();
    toolTip.image = m_notificationItem->d->serializedToolTipIcon;
    toolTip.title = m_notificationItem->toolTipTitle();
    toolTip.subTitle = m_notificationItem->toolTipSubTitle();

    return toolTip;
}

//Interaction

void KNotificationItemDBus::ContextMenu(int x, int y)
{
    //TODO: nicer placement, possible?
    if (m_notificationItem->d->menu) {
        m_notificationItem->d->menu->setWindowFlags(Qt::Window);
        m_notificationItem->d->menu->popup(QPoint(x,y));
        KWindowSystem::setState(m_notificationItem->d->menu->winId(), NET::SkipTaskbar|NET::SkipPager|NET::KeepAbove);
        KWindowSystem::setType(m_notificationItem->d->menu->winId(), NET::Menu);
        KWindowSystem::forceActiveWindow(m_notificationItem->d->menu->winId());
    }
}

void KNotificationItemDBus::Activate(int x, int y)
{
    m_notificationItem->activate(QPoint(x,y));
}

void KNotificationItemDBus::SecondaryActivate(int x, int y)
{
    emit m_notificationItem->secondaryActivateRequested(QPoint(x,y));
}

void KNotificationItemDBus::Scroll(int delta, const QString &orientation)
{
    Qt::Orientation dir = (orientation.toLower() == "horizontal" ? Qt::Horizontal : Qt::Vertical);
    emit m_notificationItem->scrollRequested(delta, dir);
}



#include "knotificationitemdbus_p.moc"
