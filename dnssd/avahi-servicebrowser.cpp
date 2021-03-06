/* This file is part of the KDE project
 *
 * Copyright (C) 2004 Jakub Stachowski <qbast@go2.pl>
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

#include "avahi-servicebrowser_p.h"
#include <QtCore/QStringList>
#include "servicebrowser.h"
#include "avahi_servicebrowser_interface.h"
#include "avahi_server_interface.h"
#include <QtCore/QHash>
#include <QtNetwork/QHostAddress>
#ifndef KDE_USE_FINAL
Q_DECLARE_METATYPE(QList<QByteArray>)
#endif
namespace DNSSD
{

ServiceBrowser::ServiceBrowser(const QString& type,bool autoResolve,const QString& domain, const QString& subtype)
	:d(new ServiceBrowserPrivate(this))
{
	d->m_type=type;
	d->m_subtype=subtype;
	d->m_autoResolve=autoResolve;
	d->m_domain=domain;
	d->m_timer.setSingleShot(true);
}

ServiceBrowser::State ServiceBrowser::isAvailable()
{
	org::freedesktop::Avahi::Server s("org.freedesktop.Avahi","/",QDBusConnection::systemBus());
	QDBusReply<int> rep= s.GetState();
	return (rep.isValid() && rep.value()==2) ? Working:Stopped;
}
ServiceBrowser::~ServiceBrowser()
{
    delete d;
}

bool ServiceBrowser::isAutoResolving() const
{
    return d->m_autoResolve;
}

void ServiceBrowser::startBrowse()
{
	if (d->m_running) return;
	org::freedesktop::Avahi::Server s("org.freedesktop.Avahi","/",QDBusConnection::systemBus());
	QString fullType=d->m_type;
	if (!d->m_subtype.isEmpty()) fullType=d->m_subtype+"._sub."+d->m_type;
	QDBusReply<QDBusObjectPath> rep=s.ServiceBrowserNew(-1, -1, fullType, domainToDNS(d->m_domain),0);
	
	if (!rep.isValid()) {
	    emit finished();
	    return;
	}
	d->m_running=true;
	d->m_browserFinished=true;
	org::freedesktop::Avahi::ServiceBrowser *b=new org::freedesktop::Avahi::ServiceBrowser("org.freedesktop.Avahi",rep.value().path(),
	    QDBusConnection::systemBus());
	connect(b,SIGNAL(ItemNew(int,int,const QString&,const QString&,const QString&,uint)),d, 
	    SLOT(gotNewService(int,int,const QString&,const QString&,const QString&, uint)));
	connect(b,SIGNAL(ItemRemove(int,int,const QString&,const QString&,const QString&,uint)),d, 
	    SLOT(gotRemoveService(int,int,const QString&,const QString&,const QString&, uint)));
	connect(b,SIGNAL(AllForNow()),d,SLOT(browserFinished()));
	d->m_browser=b;
	connect(&d->m_timer,SIGNAL(timeout()), d, SLOT(browserFinished()));
	d->m_timer.start(TIMEOUT_LAN);
}

void ServiceBrowserPrivate::serviceResolved(bool success)
{
	QObject* sender_obj = const_cast<QObject*>(sender());
	RemoteService* svr = static_cast<RemoteService*>(sender_obj);
	disconnect(svr,SIGNAL(resolved(bool)),this,SLOT(serviceResolved(bool)));
	QList<RemoteService::Ptr>::Iterator it = m_duringResolve.begin();
	QList<RemoteService::Ptr>::Iterator itEnd = m_duringResolve.end();
	while ( it!= itEnd && svr!= (*it).data()) ++it;
	if (it != itEnd) {
		if (success) {
		  	m_services+=(*it);
			emit m_parent->serviceAdded(RemoteService::Ptr(svr));
		}
		m_duringResolve.erase(it);
		queryFinished();
	}
}

RemoteService::Ptr ServiceBrowserPrivate::find(RemoteService::Ptr s) const
{
    Q_FOREACH (const RemoteService::Ptr& i, m_services) if (*s==*i) return i;
    Q_FOREACH (const RemoteService::Ptr& i, m_duringResolve) if (*s==*i) return i;
    return s;
}

void ServiceBrowserPrivate::gotNewService(int,int,const QString& name, const QString& type, const QString& domain, uint)
{
	m_timer.start(TIMEOUT_LAN);
	RemoteService::Ptr svr(new RemoteService(name, type,domain));
	if (m_autoResolve) {
		connect(svr.data(),SIGNAL(resolved(bool )),this,SLOT(serviceResolved(bool )));
		m_duringResolve+=svr;
		svr->resolveAsync();
	} else	{
		m_services+=svr;
		emit m_parent->serviceAdded(svr);
	}
}

void ServiceBrowserPrivate::gotRemoveService(int,int,const QString& name, const QString& type, const QString& domain, uint)
{
	m_timer.start(TIMEOUT_LAN);
	RemoteService::Ptr svr=find(RemoteService::Ptr(new RemoteService(name, type,domain)));
	emit m_parent->serviceRemoved(svr);
	m_services.removeAll(svr);
}
void ServiceBrowserPrivate::browserFinished()
{
    m_timer.stop();
    m_browserFinished=true;
    queryFinished();
}

void ServiceBrowserPrivate::queryFinished()
{
	if (!m_duringResolve.count() && m_browserFinished) emit m_parent->finished();
}


QList<RemoteService::Ptr> ServiceBrowser::services() const
{
	return d->m_services;
}

void ServiceBrowser::virtual_hook(int, void*)
{}

QHostAddress ServiceBrowser::resolveHostName(const QString &hostname)
{
	org::freedesktop::Avahi::Server s("org.freedesktop.Avahi","/",QDBusConnection::systemBus());

	int protocol = 0;
	QString name;
	int aprotocol = 0;
	QString address;
	uint flags = 0;

	QDBusReply<int> reply = s.ResolveHostName(-1, -1, hostname, 0, (unsigned int ) 0, protocol, name, aprotocol, address, flags);

	if (reply.isValid())
		return QHostAddress(address);
	else
		return QHostAddress();
}

QString ServiceBrowser::getLocalHostName()
{
	org::freedesktop::Avahi::Server s("org.freedesktop.Avahi","/",QDBusConnection::systemBus());

	QDBusReply<QString> reply = s.GetHostName();

	if (reply.isValid())
		return reply.value();
	else
		return QString();
}

}

#include "servicebrowser.moc"
#include "avahi-servicebrowser_p.moc"
