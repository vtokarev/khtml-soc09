/*  This file is part of the KDE project
    Copyright (C) 2006 Davide Bettio <davide.bettio@kdemail.net>

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

#include "fakestorage.h"

using namespace Solid::Backends::Fake;

FakeStorage::FakeStorage(FakeDevice *device)
    : FakeBlock(device)
{

}

FakeStorage::~FakeStorage()
{

}

Solid::StorageDrive::Bus FakeStorage::bus() const
{
    QString bus = fakeDevice()->property("bus").toString();

    if (bus=="ide")
    {
        return Solid::StorageDrive::Ide;
    }
    else if (bus=="usb")
    {
        return Solid::StorageDrive::Usb;
    }
    else if (bus=="ieee1394")
    {
        return Solid::StorageDrive::Ieee1394;
    }
    else if (bus=="scsi")
    {
        return Solid::StorageDrive::Scsi;
    }
    else if (bus=="sata")
    {
        return Solid::StorageDrive::Sata;
    }
    else
    {
        return Solid::StorageDrive::Platform;
    }
}

Solid::StorageDrive::DriveType FakeStorage::driveType() const
{
    QString type = fakeDevice()->property("major").toString();

    if (type=="disk")
    {
        return Solid::StorageDrive::HardDisk;
    }
    else if (type=="cdrom")
    {
        return Solid::StorageDrive::CdromDrive;
    }
    else if (type=="floppy")
    {
        return Solid::StorageDrive::Floppy;
    }
    else if (type=="tape")
    {
        return Solid::StorageDrive::Tape;
    }
    else if (type=="compact_flash")
    {
        return Solid::StorageDrive::CompactFlash;
    }
    else if (type=="memory_stick")
    {
        return Solid::StorageDrive::MemoryStick;
    }
    else if (type=="smart_media")
    {
        return Solid::StorageDrive::SmartMedia;
    }
    else if (type=="sd_mmc")
    {
        return Solid::StorageDrive::SdMmc;
    }
    else
    {
        return Solid::StorageDrive::HardDisk;
    }
}

bool FakeStorage::isRemovable() const
{
    return fakeDevice()->property("isRemovable").toBool();
}

bool FakeStorage::isHotpluggable() const
{
    return fakeDevice()->property("isHotpluggable").toBool();
}

#include "backends/fakehw/fakestorage.moc"
