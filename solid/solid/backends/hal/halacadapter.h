/*  This file is part of the KDE project
    Copyright (C) 2006 Kevin Ottens <ervin@kde.org>

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

#ifndef SOLID_BACKENDS_HAL_ACADAPTER_H
#define SOLID_BACKENDS_HAL_ACADAPTER_H

#include <solid/ifaces/acadapter.h>
#include "haldeviceinterface.h"

namespace Solid
{
namespace Backends
{
namespace Hal
{
class AcAdapter : public DeviceInterface, virtual public Solid::Ifaces::AcAdapter
{
    Q_OBJECT
    Q_INTERFACES(Solid::Ifaces::AcAdapter)

public:
    AcAdapter(HalDevice *device);
    virtual ~AcAdapter();

    virtual bool isPlugged() const;

Q_SIGNALS:
    void plugStateChanged(bool newState, const QString &udi);

private Q_SLOTS:
    void slotPropertyChanged(const QMap<QString,int> &changes);
};
}
}
}

#endif // SOLID_BACKENDS_HAL_ACADAPTER_H
