/*  This file is part of the KDE project
    Copyright (C) 2009 Harald Fernengel <harry@kdevelop.org>

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

#include "iokitmanager.h"
#include "iokitdevice.h"

#include <QtCore/qdebug.h>

#include <IOKit/IOKitLib.h>
#include <IOKit/usb/IOUSBLib.h>
#include <IOKit/network/IOEthernetInterface.h>

#include <CoreFoundation/CoreFoundation.h>

namespace Solid { namespace Backends { namespace IOKit {

class IOKitManagerPrivate
{
public:
    inline IOKitManagerPrivate()
        : port(0), source(0)
    {}

    IONotificationPortRef port;
    CFRunLoopSourceRef source;

    static const char *typeToName(Solid::DeviceInterface::Type type);
    static QStringList devicesFromRegistry(io_iterator_t it);
};

// gets all registry pathes from an iterator
QStringList IOKitManagerPrivate::devicesFromRegistry(io_iterator_t it)
{
    QStringList result;
    io_object_t obj;
    io_string_t pathName;
    while ((obj = IOIteratorNext(it))) {
        kern_return_t ret = IORegistryEntryGetPath(obj, kIOServicePlane, pathName);
        if (ret != KERN_SUCCESS) {
            qWarning() << Q_FUNC_INFO << "IORegistryEntryGetPath failed";
            continue;
        }
        result += QString::fromUtf8(pathName);
        ret = IOObjectRelease(obj);
        if (ret != KERN_SUCCESS) {
            // very unlikely to happen - keep it a qDebug just in case.
            // compiler will nuke this code in release builds.
            qDebug() << Q_FUNC_INFO << "Unable to release object reference";
        }
    }
    IOObjectRelease(it);

    return result;
}

const char *IOKitManagerPrivate::typeToName(Solid::DeviceInterface::Type type)
{
    switch (type) {
    case Solid::DeviceInterface::Unknown:
        return 0;
    case Solid::DeviceInterface::NetworkInterface:
        return kIOEthernetInterfaceClass;
    case Solid::DeviceInterface::Processor:
        return "AppleACPICPU";
    case Solid::DeviceInterface::SerialInterface:
        return "IOSerialBSDClient";
    case Solid::DeviceInterface::Battery:
        return "AppleSmartBattery";

    //Solid::DeviceInterface::GenericInterface:
    //Solid::DeviceInterface::Block:
    //Solid::DeviceInterface::StorageAccess:
    //Solid::DeviceInterface::StorageDrive:
    //Solid::DeviceInterface::OpticalDrive:
    //Solid::DeviceInterface::StorageVolume:
    //Solid::DeviceInterface::OpticalDisc:
    //Solid::DeviceInterface::Camera:
    //Solid::DeviceInterface::PortableMediaPlayer:
    //Solid::DeviceInterface::NetworkInterface:
    //Solid::DeviceInterface::AcAdapter:
    //Solid::DeviceInterface::Button:
    //Solid::DeviceInterface::AudioInterface:
    //Solid::DeviceInterface::DvbInterface:
    //Solid::DeviceInterface::Video:
    }

    return 0;
}

IOKitManager::IOKitManager(QObject *parent)
    : Solid::Ifaces::DeviceManager(parent), d(new IOKitManagerPrivate)
{
    d->port = IONotificationPortCreate(kIOMasterPortDefault);
    if (!d->port) {
        qWarning() << Q_FUNC_INFO << "Unable to create notification port";
        return;
    }

    d->source = IONotificationPortGetRunLoopSource(d->port);
    if (!d->source) {
        qWarning() << Q_FUNC_INFO << "Unable to create notification source";
        return;
    }

    CFRunLoopAddSource(CFRunLoopGetCurrent(), d->source, kCFRunLoopDefaultMode);
}

IOKitManager::~IOKitManager()
{
    if (d->source)
        CFRunLoopRemoveSource(CFRunLoopGetCurrent(), d->source, kCFRunLoopDefaultMode);
    if (d->port)
        IONotificationPortDestroy(d->port);

    delete d;
}

QStringList IOKitManager::allDevices()
{
    // use an IORegistry Iterator to iterate over all devices in the service plane

    io_iterator_t it;
    kern_return_t ret = IORegistryCreateIterator(
            kIOMasterPortDefault,
            kIOServicePlane,
            kIORegistryIterateRecursively,
            &it);
    if (ret != KERN_SUCCESS) {
        qWarning() << Q_FUNC_INFO << "unable to create iterator";
        return QStringList();
    }

    return IOKitManagerPrivate::devicesFromRegistry(it);
}

QStringList IOKitManager::devicesFromQuery(const QString &parentUdi,
                                           Solid::DeviceInterface::Type type)
{
    QStringList result;

    if (type == Solid::DeviceInterface::Unknown) {
        // match all device interfaces
        result = allDevices();
    } else {
        const char *deviceClassName = IOKitManagerPrivate::typeToName(type);
        if (!deviceClassName)
            return QStringList();

        CFMutableDictionaryRef matchingDict = IOServiceMatching(deviceClassName);

        if (!matchingDict)
            return QStringList();

        io_iterator_t it = 0;

        // note - IOServiceGetMatchingServices dereferences the dict
        kern_return_t ret = IOServiceGetMatchingServices(
                kIOMasterPortDefault,
                matchingDict,
                &it);

        result = IOKitManagerPrivate::devicesFromRegistry(it);
    }

    // if the parentUdi is an empty string, return all matches
    if (parentUdi.isEmpty())
        return result;

    // return only matches that start with the parent's UDI
    QStringList filtered;
    foreach (QString udi, result) {
        if (udi.startsWith(parentUdi))
            filtered += udi;
    }

    return filtered;
}

QObject *IOKitManager::createDevice(const QString &udi)
{
    io_registry_entry_t entry = IORegistryEntryFromPath(
            kIOMasterPortDefault,
            udi.toLocal8Bit().constData());

    // we have to do IOObjectConformsTo - comparing the class names is not good enough
    //if (IOObjectConformsTo(entry, kIOEthernetInterfaceClass)) {
    //}

    if (entry == MACH_PORT_NULL)
        return 0;

    return new IOKitDevice(udi, entry);
}

}}} // namespaces

