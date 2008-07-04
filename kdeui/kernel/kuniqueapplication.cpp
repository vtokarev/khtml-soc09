/* This file is part of the KDE libraries
    Copyright (c) 1999 Preston Brown <pbrown@kde.org>

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

#include "kuniqueapplication.h"
#include "kuniqueapplication_p.h"
#include <kmainwindow.h>

#include <config.h>

#include <sys/types.h>
#include <sys/wait.h>

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#include <QtCore/QFile>
#include <QtCore/QList>
#include <QtCore/QTimer>
#include <QtDBus/QtDBus>

#include <kcmdlineargs.h>
#include <kstandarddirs.h>
#include <kaboutdata.h>
#include <kconfiggroup.h>

#if defined Q_WS_X11
#include <kstartupinfo.h>
#endif

/* I don't know why, but I end up with complaints about
   a forward-declaration of QWidget in the activeWidow()->show
   call below on Qt/Mac if I don't include this here... */
#include <QWidget>

#include <kconfig.h>
#include "kdebug.h"

#if defined Q_WS_X11
#include <netwm.h>
#include <X11/Xlib.h>
#define DISPLAY "DISPLAY"
#else
#  ifdef Q_WS_QWS
#    define DISPLAY "QWS_DISPLAY"
#  else
#    define DISPLAY "DISPLAY"
#  endif
#endif

#ifdef Q_WS_MAC
#include <kkernel_mac.h>
#endif

bool KUniqueApplication::Private::s_nofork = false;
bool KUniqueApplication::Private::s_multipleInstances = false;
bool s_kuniqueapplication_startCalled = false;
bool KUniqueApplication::Private::s_handleAutoStarted = false;
#ifdef Q_WS_WIN
QString KUniqueApplication::Private::s_dbusServiceName;
/* private helpers from kapplication_win.cpp */
void KApplication_activateWindowForProcess( const QString& executableName );
bool KApplication_dbusIsPatched();
#endif

void
KUniqueApplication::addCmdLineOptions()
{
  KCmdLineOptions kunique_options;
  kunique_options.add("nofork", ki18n("Do not run in the background."));
#ifdef Q_WS_MACX
  kunique_options.add("psn", ki18n("Internally added if launched from Finder"));
#endif
  KCmdLineArgs::addCmdLineOptions(kunique_options, KLocalizedString(), "kuniqueapp", "kde");
}

static QDBusConnectionInterface *tryToInitDBusConnection()
{
    // Check the D-Bus connection health
    QDBusConnectionInterface* dbusService = 0;
    if (!QDBusConnection::sessionBus().isConnected() || !(dbusService = QDBusConnection::sessionBus().interface()))
    {
        kError() << "KUniqueApplication: Cannot find the D-Bus session server" << endl;
        ::exit(255);
    }
    return dbusService;
}

bool KUniqueApplication::start()
{
	return start(0);
}

bool
KUniqueApplication::start(StartFlags flags)
{
  if( s_kuniqueapplication_startCalled )
    return true;
  s_kuniqueapplication_startCalled = true;

  addCmdLineOptions(); // Make sure to add cmd line options
#ifdef Q_WS_WIN
  Private::s_nofork = true;
#else
  KCmdLineArgs *args = KCmdLineArgs::parsedArgs("kuniqueapp");
#ifdef Q_WS_MACX
  // avoid focus loss caused by extra fork when launched from Finder
  if(args->isSet("psn"))
     Private::s_nofork = true;
  else
#endif
  Private::s_nofork = !args->isSet("fork");
  delete args;
#endif

  QString appName = KCmdLineArgs::aboutData()->appName();
  const QStringList parts = KCmdLineArgs::aboutData()->organizationDomain().split(QLatin1Char('.'), QString::SkipEmptyParts);
  if (parts.isEmpty())
     appName.prepend(QLatin1String("local."));
  else
     foreach (const QString& s, parts)
     {
        appName.prepend(QLatin1Char('.'));
        appName.prepend(s);
     }

#ifdef Q_WS_MAC
  mac_initialize_dbus();
#endif

  bool forceNewProcess = Private::s_multipleInstances || flags & NonUniqueInstance;

  if (Private::s_nofork)
  {
     QDBusConnectionInterface* dbusService = tryToInitDBusConnection();

     QString pid = QString::number(getpid());
     if (forceNewProcess)
        appName = appName + '-' + pid;

     // Check to make sure that we're actually able to register with the D-Bus session
     // server.
     bool registered;
#ifdef Q_WS_WIN
     if (KApplication_dbusIsPatched())
       registered = dbusService->registerService(appName + ".unique-" + pid) == QDBusConnectionInterface::ServiceRegistered;
     else
       registered = dbusService->registerService(appName) == QDBusConnectionInterface::ServiceRegistered;
#else
     registered = dbusService->registerService(appName) == QDBusConnectionInterface::ServiceRegistered;
#endif
     if (!registered)
     {
        kError() << "KUniqueApplication: Can't setup D-Bus service. Probably already running."
                 << endl;
#ifdef Q_WS_WIN
        // on Windows we'll restart the app (at least in KMail, KOrganizer, Konact...)
        return false;
#else
        ::exit(255);
#endif
     }
#ifdef Q_WS_WIN
     Private::s_dbusServiceName = appName;
#endif

     // We'll call newInstance in the constructor. Do nothing here.
     return true;

#ifdef Q_WS_MACX
  } else {
    mac_fork_and_reexec_self();
#endif

  }

#ifndef Q_WS_WIN
  int fd[2];
  signed char result;
  if (0 > pipe(fd))
  {
     kError() << "KUniqueApplication: pipe() failed!" << endl;
     ::exit(255);
  }
  int fork_result = fork();
  switch(fork_result) {
  case -1:
     kError() << "KUniqueApplication: fork() failed!" << endl;
     ::exit(255);
     break;
  case 0:
     {
        // Child

        QDBusConnectionInterface* dbusService = tryToInitDBusConnection();
        ::close(fd[0]);
        if (forceNewProcess)
           appName.append("-").append(QString::number(getpid()));

        QDBusReply<QDBusConnectionInterface::RegisterServiceReply> reply =
            dbusService->registerService(appName);
        if (!reply.isValid())
        {
           kError() << "KUniqueApplication: Can't setup D-Bus service." << endl;
           result = -1;
           ::write(fd[1], &result, 1);
           ::exit(255);
        }
        if (reply == QDBusConnectionInterface::ServiceNotRegistered)
        {
           // Already running. Ok.
           result = 0;
           ::write(fd[1], &result, 1);
           ::close(fd[1]);
           return false;
        }

#ifdef Q_WS_X11
         KStartupInfoId id;
         if( kapp != NULL ) // KApplication constructor unsets the env. variable
             id.initId( kapp->startupId());
         else
             id = KStartupInfo::currentStartupIdEnv();
         if( !id.none())
         { // notice about pid change
            Display* disp = XOpenDisplay( NULL );
            if( disp != NULL ) // use extra X connection
            {
               KStartupInfoData data;
               data.addPid( getpid());
               KStartupInfo::sendChangeX( disp, id, data );
               XCloseDisplay( disp );
            }
         }
#else //FIXME(E): Implement
#endif
     }
     result = 0;
     ::write(fd[1], &result, 1);
     ::close(fd[1]);
     return true; // Finished.
  default:
     // Parent

     if (forceNewProcess)
        appName.append("-").append(QString::number(fork_result));
     ::close(fd[1]);

     Q_FOREVER
     {
       int n = ::read(fd[0], &result, 1);
       if (n == 1) break;
       if (n == 0)
       {
          kError() << "KUniqueApplication: Pipe closed unexpectedly." << endl;
          ::exit(255);
       }
       if (errno != EINTR)
       {
          kError() << "KUniqueApplication: Error reading from pipe." << endl;
          ::exit(255);
       }
     }
     ::close(fd[0]);

     if (result != 0)
        ::exit(result); // Error occurred in child.

#endif
     QDBusConnectionInterface* dbusService = tryToInitDBusConnection();
     if (!dbusService->isServiceRegistered(appName))
     {
        kError() << "KUniqueApplication: Registering failed!" << endl;
     }

     QByteArray saved_args;
     QDataStream ds(&saved_args, QIODevice::WriteOnly);
     KCmdLineArgs::saveAppArgs(ds);

     QByteArray new_asn_id;
#if defined Q_WS_X11
     KStartupInfoId id;
     if( kapp != NULL ) // KApplication constructor unsets the env. variable
         id.initId( kapp->startupId());
     else
         id = KStartupInfo::currentStartupIdEnv();
     if( !id.none())
         new_asn_id = id.id();
#endif

     QDBusInterface iface(appName, "/MainApplication", "org.kde.KUniqueApplication", QDBusConnection::sessionBus());
     QDBusReply<int> reply;
     if (!iface.isValid() || !(reply = iface.call("newInstance", new_asn_id, saved_args)).isValid())
     {
       QDBusError err = iface.lastError();
        kError() << "Communication problem with " << KCmdLineArgs::aboutData()->appName() << ", it probably crashed." << endl
                 << "Error message was: " << err.name() << ": \"" << err.message() << "\"" << endl;
        ::exit(255);
     }
#ifndef Q_WS_WIN
     ::exit(reply);
     break;
  }
#endif
  return false; // make insure++ happy
}


KUniqueApplication::KUniqueApplication(bool GUIenabled, bool configUnique)
  : KApplication( GUIenabled, Private::initHack( configUnique )),
    d(new Private(this))
{
  d->processingRequest = false;
  d->firstInstance = true;

  // the sanity checking happened in initHack
  new KUniqueApplicationAdaptor(this);

  if (Private::s_nofork)
    // Can't call newInstance directly from the constructor since it's virtual...
    QTimer::singleShot( 0, this, SLOT(_k_newInstanceNoFork()) );
}


#ifdef Q_WS_X11
KUniqueApplication::KUniqueApplication(Display *display, Qt::HANDLE visual,
		Qt::HANDLE colormap, bool configUnique)
  : KApplication( display, visual, colormap, Private::initHack( configUnique )),
    d(new Private(this))
{
  d->processingRequest = false;
  d->firstInstance = true;

  // the sanity checking happened in initHack
  new KUniqueApplicationAdaptor(this);

  if (Private::s_nofork)
    // Can't call newInstance directly from the constructor since it's virtual...
    QTimer::singleShot( 0, this, SLOT(_k_newInstanceNoFork()) );
}
#endif


KUniqueApplication::~KUniqueApplication()
{
#ifdef Q_WS_WIN
  // work around for KUniqueApplication being not completely implemented on windows
  QDBusConnectionInterface* dbusService;
  if (QDBusConnection::sessionBus().isConnected()
    && (dbusService = QDBusConnection::sessionBus().interface()))
  {
    dbusService->unregisterService(Private::s_dbusServiceName);
  }
#endif

  delete d;
}

// this gets called before even entering QApplication::QApplication()
KComponentData KUniqueApplication::Private::initHack(bool configUnique)
{
  KComponentData cData(KCmdLineArgs::aboutData());
  if (configUnique)
  {
     KConfigGroup cg(cData.config(), "KDE");
     s_multipleInstances = cg.readEntry("MultipleInstances", false);
  }
  if( !KUniqueApplication::start())
     // Already running
     ::exit( 0 );
  return cData;
}

void KUniqueApplication::Private::_k_newInstanceNoFork()
{
  s_handleAutoStarted = false;
  q->newInstance();
  firstInstance = false;
#if defined Q_WS_X11
  // KDE4 remove
  // A hack to make startup notification stop for apps which override newInstance()
  // and reuse an already existing window there, but use KWindowSystem::activateWindow()
  // instead of KStartupInfo::setNewStartupId(). Therefore KWindowSystem::activateWindow()
  // for now sets this flag. Automatically ending startup notification always
  // would cause problem if the new window would show up with a small delay.
  if( s_handleAutoStarted )
      KStartupInfo::handleAutoAppStartedSending();
#endif
  // What to do with the return value ?
}

bool KUniqueApplication::restoringSession()
{
  return d->firstInstance && isSessionRestored();
}

int KUniqueApplication::newInstance()
{
    if (!d->firstInstance) {
        QList<KMainWindow*> allWindows = KMainWindow::memberList();
        if (!allWindows.isEmpty()) {
            // This method is documented to only work for applications
            // with only one mainwindow.
            KMainWindow* mainWindow = allWindows.first();
            if (mainWindow) {
                mainWindow->show();
#if defined Q_WS_X11
                // This is the line that handles window activation if necessary,
                // and what's important, it does it properly. If you reimplement newInstance(),
                // and don't call the inherited one, use this (but NOT when newInstance()
                // is called for the first time, like here).
                KStartupInfo::setNewStartupId(mainWindow, startupId());
#endif
            }
        }
    }
    return 0; // do nothing in default implementation
}

void KUniqueApplication::setHandleAutoStarted()
{
    Private::s_handleAutoStarted = false;
}

#include "kuniqueapplication.moc"
#include "kuniqueapplication_p.moc"
