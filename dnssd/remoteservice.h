/* This file is part of the KDE project
 *
 * Copyright (C) 2004, 2005 Jakub Stachowski <qbast@go2.pl>
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

#ifndef DNSSDREMOTESERVICE_H
#define DNSSDREMOTESERVICE_H

#include <QtCore/QObject>
#include <QtCore/QMetaType>
#include <dnssd/servicebase.h>

namespace DNSSD
{
class RemoteServicePrivate;

/**
\class RemoteService remoteservice.h DNSSD/RemoteService

RemoteService class allows to resolve service announced on remote machine. In most cases objects
of this class are created by ServiceBrowser, but it is not required. Only fields valid before 
service is resolved are name, type.and domain. 
 
 
@short class representing service announced on remote machine.
@author Jakub Stachowski
 */
class KDNSSD_EXPORT RemoteService : public QObject, public ServiceBase
{
	Q_OBJECT
public:
	typedef KSharedPtr<RemoteService> Ptr;
	
	/**
	Creates unresolved remote service with given name, type and domain.
	 */
	RemoteService(const QString& name,const QString& type,const QString& domain);
	
	virtual ~RemoteService();
	
	/**
	Resolves host name and port of service. Host name is not resolved into numeric
	address - use KResolver for that. Signal resolved(bool) will be emitted 
	when finished or even before return of this function - in case of immediate failure.
	 */
	void resolveAsync();
	
	/**
	Synchronous version of resolveAsync(). Note that resolved(bool) is emitted 
	before this function returns, 
	@return true is successful
	 */
	bool resolve();
	
	/**
	Returns true if service has been successfully resolved
	 */
	bool isResolved() const;
	
Q_SIGNALS:
	/**
	Emitted when resolving is complete. Parameter is set to true if it was successful.
	If operating in asynchronous mode this signal can be emitted several times (when 
	service change)
	 */
	void resolved(bool);

protected:
	virtual void virtual_hook(int id, void *data);
private:
	friend class RemoteServicePrivate;

};

}

Q_DECLARE_METATYPE(DNSSD::RemoteService::Ptr)

#endif
