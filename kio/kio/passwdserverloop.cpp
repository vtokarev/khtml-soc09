/*
 *  This file is part of the KDE libraries
 *  Copyright (c) 2009 Michael Leupold <lemma@confuego.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License version 2 as published by the Free Software Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 *
 **/

#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusConnectionInterface>

#include "passwdserverloop_p.h"

namespace KIO
{

PasswdServerLoop::PasswdServerLoop() : m_seqNr(-1)
{
    connect(QDBusConnection::sessionBus().interface(),
            SIGNAL(serviceOwnerChanged(QString, QString, QString)),
            this,
            SLOT(slotServiceOwnerChanged(QString, QString, QString)));
}

PasswdServerLoop::~PasswdServerLoop()
{
}

bool PasswdServerLoop::waitForResult(qlonglong requestId)
{
    m_requestId = requestId;
    m_seqNr = -1;
    m_authInfo = AuthInfo();
    return (exec() == 0);
}

qlonglong PasswdServerLoop::seqNr() const
{
    return m_seqNr;
}

const AuthInfo &PasswdServerLoop::authInfo() const
{
    return m_authInfo;
}

void PasswdServerLoop::slotQueryResult(qlonglong requestId, qlonglong seqNr,
                                       const KIO::AuthInfo &authInfo)
{
    if (m_requestId == requestId) {
        m_seqNr = seqNr;
        m_authInfo = authInfo;
        exit(0);
    }
}

void PasswdServerLoop::slotServiceOwnerChanged(const QString &name, const QString &oldOwner,
                                               const QString &newOwner)
{
    Q_UNUSED(oldOwner);

    if (newOwner.isEmpty() && name == "org.kde.kpasswdserver") {
        exit(-1);
    }
}
    
}

#include "passwdserverloop_p.moc"
