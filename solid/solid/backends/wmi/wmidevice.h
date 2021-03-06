/*  This file is part of the KDE project
    Copyright (C) 2005,2006 Kevin Ottens <ervin@kde.org>

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

#ifndef SOLID_BACKENDS_WMI_WMIDEVICE_H
#define SOLID_BACKENDS_WMI_WMIDEVICE_H

#include <solid/ifaces/device.h>

namespace Solid
{
namespace Backends
{
namespace Wmi
{
class WmiManager;
class WmiDevicePrivate;

struct ChangeDescription
{
    QString key;
    bool added;
    bool removed;
};

class WmiDevice : public Solid::Ifaces::Device
{
    Q_OBJECT

public:
    WmiDevice(const QString &udi);
    virtual ~WmiDevice();

    virtual QString udi() const;
    virtual QString parentUdi() const;

    virtual QString vendor() const;
    virtual QString product() const;
    virtual QString icon() const;
    virtual QString description() const;

    virtual bool isValid() const;
    
    virtual QVariant property(const QString &key) const;

    virtual QMap<QString, QVariant> allProperties() const;

    virtual bool propertyExists(const QString &key) const;

    virtual bool queryDeviceInterface(const Solid::DeviceInterface::Type &type) const;
    virtual QObject *createDeviceInterface(const Solid::DeviceInterface::Type &type);
    
    static QStringList generateUDIList(const Solid::DeviceInterface::Type &type);
    static bool exists(const QString &udi);

Q_SIGNALS:
    void propertyChanged(const QMap<QString,int> &changes);
    void conditionRaised(const QString &condition, const QString &reason);

private Q_SLOTS:
    void slotPropertyModified(int count, const QList<ChangeDescription> &changes);
    void slotCondition(const QString &condition, const QString &reason);

private:
    WmiDevicePrivate *d;
    friend class WmiDevicePrivate;
};
}
}
}

#endif // SOLID_BACKENDS_WMI_WMIDEVICE_H
