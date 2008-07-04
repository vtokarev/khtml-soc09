/* This file is part of the KDE libraries
    Copyright (c) 2006 Thiago Macieira <thiagokde.org>

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

#ifndef KUNIQUEAPPLICATION_P_H
#define KUNIQUEAPPLICATION_P_H

#include <QtDBus/QtDBus>
#include <kcmdlineargs.h>

class KUniqueApplication::Private
{
public:
    Private(KUniqueApplication *q)
        : q(q)
    {
    }

   void _k_newInstanceNoFork();

   static KComponentData initHack(bool configUnique);

   KUniqueApplication *q;
   bool processingRequest;
   bool firstInstance;

   static bool s_nofork;
   static bool s_multipleInstances;
   static bool s_handleAutoStarted;

#ifdef Q_WS_WIN
   static QString s_dbusServiceName;
#endif
};

class KUniqueApplicationAdaptor: public QDBusAbstractAdaptor
{
  Q_OBJECT
  Q_CLASSINFO("D-Bus Interface", "org.kde.KUniqueApplication")
public:
  KUniqueApplicationAdaptor(KUniqueApplication *parent)
    : QDBusAbstractAdaptor(parent)
  { }

  inline KUniqueApplication *parent() const
  { return static_cast<KUniqueApplication *>(QDBusAbstractAdaptor::parent()); }

public Q_SLOTS:
  int newInstance(const QByteArray &asn_id = QByteArray(), const QByteArray &args = QByteArray())
  {
    if (!asn_id.isEmpty())
      parent()->setStartupId(asn_id);

    // This hook allows the application to set up KCmdLineArgs using addCmdLineOptions
    // before we load the app args. Normally not necessary, but needed by kontact
    // since it switches to other sets of options when called as e.g. kmail or korganizer
    QMetaObject::invokeMethod(parent(), "loadCommandLineOptionsForNewInstance");

    QDataStream ds(args);
    KCmdLineArgs::loadAppArgs(ds);

    int ret = parent()->newInstance();
    // Must be done out of the newInstance code, in case it is overloaded
    parent()->d->firstInstance = false;
    return ret;
  }
};

#endif

/*
 * Local variables:
 *  c-basic-offset: 2
 *  indent-tabs-mode: nil
 * End:
 */
