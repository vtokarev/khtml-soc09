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
 */

#ifndef KPASSWDSERVER_P_H
#define KPASSWDSERVER_P_H

#include <QObject>

class QString;
class OrgKdeKPasswdServerInterface;

namespace KIO
{
    class AuthInfo;

/**
 * Interface class for kpasswdserver.
 * @internal
 * @remarks This is currently only supposed to be used by KIO::SlaveBase
 *          but might be reused as public API in the future.
 */
class KPasswdServer : public QObject
{
    Q_OBJECT

public:
    KPasswdServer();
    ~KPasswdServer();

    /**
     * Check if kpasswdserver has cached authentication information regarding
     * an AuthInfo object.
     * @param info information to check cache for
     * @param windowId used as parent for dialogs
     * @param usertime FIXME: I'd like to know as well :)
     * @return kpasswdserver's sequence number or -1 on error
     * @remarks info will contain the results of the check. To see if
     *          information was retrieved, check info.isModified().
     */
    qlonglong checkAuthInfo(KIO::AuthInfo &info, qlonglong windowId,
                            qlonglong usertime);

    /**
     * Let kpasswdserver ask the user for authentication information.
     * @param info information to query the user for
     * @param errorMsg error message that will be displayed to the user
     * @param seqNr sequence number to assign to this request
     * @param windowId used as parent for dialogs
     * @param usertime FIXME: I'd like to know as well :)
     * @return kpasswdserver's sequence number or -1 on error
     * @remarks info will contain the results of the check. To see if
     *          information was retrieved, check info.isModified().
     */
    qlonglong queryAuthInfo(KIO::AuthInfo &info, const QString &errorMsg,
                            qlonglong windowId, qlonglong seqNr,
                            qlonglong usertime);
    
    /**
     * Manually add authentication information to kpasswdserver's cache.
     * @param info information to add
     * @param windowId used as parent window for dialogs
     */
    void addAuthInfo(const KIO::AuthInfo &info, qlonglong windowId);

    /**
     * Manually remove authentication information from kpasswdserver's cache.
     * @param host hostname of the information to remove
     * @param protocol protocol to remove information for
     * @param user username to remove information for
     */
    void removeAuthInfo(const QString &host, const QString &protocol,
                        const QString &user);

private:
    /**
     * Legacy version of checkAuthInfo provided for compatibility with
     * old kpasswdserver.
     * @remarks automatically called by checkAuthInfo if needed.
     */
    qlonglong legacyCheckAuthInfo(KIO::AuthInfo &info, qlonglong windowId,
                                  qlonglong usertime);
    
    /**
     * Legacy version of queryAuthInfo provided for compatibility with
     * old kpasswdserver.
     * @remarks automatically called by queryAuthInfo if needed.
     */
    qlonglong legacyQueryAuthInfo(KIO::AuthInfo &info, const QString &errorMsg,
                                  qlonglong windowId, qlonglong seqNr,
                                  qlonglong usertime);

    OrgKdeKPasswdServerInterface *m_interface;
};

}

#endif
