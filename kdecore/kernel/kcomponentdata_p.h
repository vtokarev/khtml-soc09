/*  This file is part of the KDE project
    Copyright (C) 2007 Matthias Kretz <kretz@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License version 2 as published by the Free Software Foundation.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

*/

#ifndef KERNEL_KCOMPONENTDATA_P_H
#define KERNEL_KCOMPONENTDATA_P_H

#include "kcomponentdata.h"
#include <QtDebug>
#include <QString>

#include <kconfig.h>
#include <kaboutdata.h>
#include <kstandarddirs.h>
#include <klocale.h>

class KComponentDataPrivate
{
public:
    KComponentDataPrivate(const KAboutData &aboutData_)
        : dirs(0),
        aboutData(aboutData_),
        syncing(false),
        refCount(1)
    {
        if (KGlobal::hasLocale())
            KGlobal::locale()->insertCatalog(aboutData.catalogName());
    }

    ~KComponentDataPrivate()
    {
        refCount = -0x00FFFFFF; //prevent a reentering of the dtor
        if (KGlobal::hasLocale())
            KGlobal::locale()->removeCatalog(aboutData.catalogName());

        sharedConfig = 0;   //delete the config object first, because it could access the standard dirs while syncing
        delete dirs;
    }

    inline void ref()
    {
        ++refCount;
        //qDebug() << refCount - 1 << "->" << refCount << kBacktrace() << endl;
    }

    inline void deref()
    {
        --refCount;
        //qDebug() << refCount + 1 << "->" << refCount << kBacktrace() << endl;
        if (refCount == 0) {
            delete this;
        } else if (refCount == 1 && sharedConfig && sharedConfig->componentData().d == this) { //sharedConfig has a reference to us
            if (sharedConfig.count() == 1) {    //we are the only class with a reference to the config object
                delete this;
            } else if (sharedConfig.count() > 0) {  //there are other references to it. 
                sharedConfig->ref.deref();  //we don't have a reference to the config object anymore, but it has still a reference to us
                                            //this breaks the circular dependencies
            }
        }
    }

    void lazyInit(const KComponentData &component);
    void configInit(const KComponentData &component);  //call this only from lazyInit()!

    KStandardDirs *dirs;
    const KAboutData aboutData;
    QString configName;
    KSharedConfig::Ptr sharedConfig;
    bool syncing;

private:
    int refCount;
    KComponentDataPrivate(const KComponentDataPrivate&);
    KComponentDataPrivate &operator=(const KComponentDataPrivate&);
};

#endif // KERNEL_KCOMPONENTDATA_P_H
// vim: sw=4 sts=4 et tw=100
