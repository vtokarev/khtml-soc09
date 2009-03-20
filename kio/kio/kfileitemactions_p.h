/* This file is part of the KDE project
   Copyright (C) 1998-2009 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or modify
   it under the terms of the GNU Library General Public License as published
   by the Free Software Foundation; either version 2 of the License or
   ( at your option ) version 3 or, at the discretion of KDE e.V.
   ( which shall act as a proxy as in section 14 of the GPLv3 ), any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KFILEITEMACTIONS_P_H
#define KFILEITEMACTIONS_P_H

#include <kfileitemlistproperties.h>
#include <kfileitem.h>
#include <kactioncollection.h>
#include <kserviceaction.h>
#include <kservice.h>
#include <QActionGroup>
#include <QObject>

typedef QList<KServiceAction> ServiceList;

class KFileItemActionsPrivate : public QObject
{
    Q_OBJECT
public:
    KFileItemActionsPrivate();
    ~KFileItemActionsPrivate();

    int insertServicesSubmenus(const QMap<QString, ServiceList>& list, QMenu* menu, bool isBuiltin);
    int insertServices(const ServiceList& list, QMenu* menu, bool isBuiltin);

    // For "open with"
    KService::List associatedApplications(const QString& traderConstraint);
    KAction* createAppAction(const KService::Ptr& service, bool singleOffer);

private Q_SLOTS:
    // For servicemenus
    void slotExecuteService(QAction* act);
    // For "open with" applications
    void slotRunApplication(QAction* act);
    void slotOpenWithDialog();

public:
    KFileItemListProperties m_props;
    QActionGroup m_executeServiceActionGroup;
    QActionGroup m_runApplicationActionGroup;
    QList<KAction*> m_ownActions;
    QWidget* m_parentWidget;
};

Q_DECLARE_METATYPE(KService::Ptr)
Q_DECLARE_METATYPE(KServiceAction)

#endif /* KFILEITEMACTIONS_P_H */

