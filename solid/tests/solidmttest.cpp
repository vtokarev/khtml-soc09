/*  This file is part of the KDE project
    Copyright (C) 2008 Kevin Ottens <ervin@kde.org>

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

#include <QtCore/QObject>
#include <QtCore/QThread>

#include <QtTest/QtTest>

#include <solid/device.h>


class SolidMtTest : public QObject
{
    Q_OBJECT
private slots:
    void testWorkerThread();
};

class WorkerThread : public QThread
{
    Q_OBJECT
protected:
    virtual void run()
    {
        Solid::Device dev("/org/freedesktop/Hal/devices/computer");
    }
};

QTEST_MAIN(SolidMtTest)

void SolidMtTest::testWorkerThread()
{
    Solid::Device dev("/org/freedesktop/Hal/devices/acpi_ADP1");
    WorkerThread *wt = new WorkerThread;
    wt->start();
    wt->wait();
    delete wt;
}

#include "solidmttest.moc"

