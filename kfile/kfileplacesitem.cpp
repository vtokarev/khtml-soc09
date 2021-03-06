/*  This file is part of the KDE project
    Copyright (C) 2007 Kevin Ottens <ervin@kde.org>

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

#include "kfileplacesitem_p.h"
#include "kfileplacesmodel.h"

#include <QtCore/QDateTime>

#include <kbookmarkmanager.h>
#include <kiconloader.h>
#include <kdirlister.h>
#include <klocale.h>
#include <solid/opticaldisc.h>
#include <solid/storageaccess.h>
#include <solid/storagevolume.h>


KFilePlacesItem::KFilePlacesItem(KBookmarkManager *manager,
                                 const QString &address,
                                 const QString &udi)
    : m_manager(manager), m_lister(0), m_folderIsEmpty(true), m_text()
{
    setBookmark(m_manager->findByAddress(address));

    if (udi.isEmpty() && m_bookmark.metaDataItem("ID").isEmpty()) {
        m_bookmark.setMetaDataItem("ID", generateNewId());
    } else if (udi.isEmpty()) {
        if (hasFullIcon(m_bookmark)) {
            // TODO if this is only for the trash, it would be much faster to just read trashrc
            m_lister = new KDirLister(this);
            m_lister->setAutoErrorHandlingEnabled(false, 0); // don't bother the user if trash:/ doesn't exist
            m_lister->setDelayedMimeTypes(true); // we don't need the mimetypes, so don't penalize other KDirLister users
            connect(m_lister, SIGNAL(completed()),
                    this, SLOT(onListerCompleted()));
            m_lister->openUrl(m_bookmark.url());
        }
    } else if (!udi.isEmpty()) {
        Solid::Device dev(udi);
        Solid::StorageAccess *access = dev.as<Solid::StorageAccess>();
        if (access!=0) {
            connect(access, SIGNAL(accessibilityChanged(bool, const QString &)),
                    this, SLOT(onAccessibilityChanged()));
        }
    }
}

KFilePlacesItem::~KFilePlacesItem()
{
}

QString KFilePlacesItem::id() const
{
    if (isDevice()) {
        return bookmark().metaDataItem("UDI");
    } else {
        return bookmark().metaDataItem("ID");
    }
}

bool KFilePlacesItem::isDevice() const
{
    return !bookmark().metaDataItem("UDI").isEmpty();
}

KBookmark KFilePlacesItem::bookmark() const
{
    return m_bookmark;
}

void KFilePlacesItem::setBookmark(const KBookmark &bookmark)
{
    m_bookmark = bookmark;

    if (bookmark.metaDataItem("isSystemItem") == "true") {
        // This context must stay as it is - the translated system bookmark names
        // are created with 'KFile System Bookmarks' as their context, so this
        // ensures the right string is picked from the catalogue.
        // (coles, 13th May 2009)

        m_text = i18nc("KFile System Bookmarks", bookmark.text().toUtf8().data());
    } else {
        m_text = bookmark.text();
    }
}

Solid::Device KFilePlacesItem::device() const
{
    if (m_device.udi().isEmpty()) {
        m_device = Solid::Device(bookmark().metaDataItem("UDI"));
        if (m_device.isValid()) {
            m_access = m_device.as<Solid::StorageAccess>();
            m_volume = m_device.as<Solid::StorageVolume>();
            m_disc = m_device.as<Solid::OpticalDisc>();
        } else {
            m_access = 0;
            m_volume = 0;
            m_disc = 0;
        }
    }
    return m_device;
}

QVariant KFilePlacesItem::data(int role) const
{
    QVariant returnData;

    if (role!=KFilePlacesModel::HiddenRole && role!=Qt::BackgroundRole && isDevice()) {
        returnData = deviceData(role);
    } else {
        returnData = bookmarkData(role);
    }

    return returnData;
}

QVariant KFilePlacesItem::bookmarkData(int role) const
{
    KBookmark b = bookmark();

    if (b.isNull()) return QVariant();

    switch (role)
    {
    case Qt::DisplayRole:
        return m_text;
    case Qt::DecorationRole:
        return KIcon(iconNameForBookmark(b));
    case Qt::BackgroundRole:
        if (b.metaDataItem("IsHidden")=="true") {
            return Qt::lightGray;
        } else {
            return QVariant();
        }
    case KFilePlacesModel::UrlRole:
        return QUrl(b.url());
    case KFilePlacesModel::SetupNeededRole:
        return false;
    case KFilePlacesModel::HiddenRole:
        return b.metaDataItem("IsHidden")=="true";
    default:
        return QVariant();
    }
}

QVariant KFilePlacesItem::deviceData(int role) const
{
    Solid::Device d = device();

    if (d.isValid()) {
        QStringList overlays;

        switch (role)
        {
        case Qt::DisplayRole:
            return d.description();
        case Qt::DecorationRole:
            if (m_access && m_access->isAccessible()) {
                overlays << "emblem-mounted";
            } else {
                overlays << QString(); // We have to guarantee the placement of the next emblem
            }
            if (m_volume && m_volume->usage()==Solid::StorageVolume::Encrypted) {
                overlays << "security-high";
            }
            return KIcon(d.icon(), 0, overlays);
        case KFilePlacesModel::UrlRole:
            if (m_access) {
                return QUrl(KUrl(m_access->filePath()));
            } else if (m_disc && (m_disc->availableContent() & Solid::OpticalDisc::Audio)!=0) {
                return QUrl("audiocd:/");
            } else {
                return QVariant();
            }
        case KFilePlacesModel::SetupNeededRole:
            if (m_access) {
                return !m_access->isAccessible();
            } else {
                return QVariant();
            }
        default:
            return QVariant();
        }
    } else {
        return QVariant();
    }
}

KBookmark KFilePlacesItem::createBookmark(KBookmarkManager *manager,
                                          const QString &label,
                                          const KUrl &url,
                                          const QString &iconName)
{
    KBookmarkGroup root = manager->root();
    if (root.isNull())
        return KBookmark();
    QString empty_icon = iconName;
    if (url==KUrl("trash:/")) {
        if (empty_icon.endsWith("-full")) {
            empty_icon.chop(5);
        } else if (empty_icon.isEmpty()) {
            empty_icon = "user-trash";
        }
    }
    KBookmark bookmark = root.addBookmark(label, url, empty_icon);
    bookmark.setMetaDataItem("ID", generateNewId());

    return bookmark;
}

KBookmark KFilePlacesItem::createSystemBookmark(KBookmarkManager *manager,
                                                const QString &untranslatedLabel,
                                                const QString &translatedLabel,
                                                const KUrl &url,
                                                const QString &iconName)
{
    Q_UNUSED(translatedLabel); // parameter is only necessary to force the caller
                               // providing a translated string for the label

    KBookmark bookmark = createBookmark(manager, untranslatedLabel, url, iconName);
    if (!bookmark.isNull())
        bookmark.setMetaDataItem("isSystemItem", "true");
    return bookmark;
}


KBookmark KFilePlacesItem::createDeviceBookmark(KBookmarkManager *manager,
                                                const QString &udi)
{
    KBookmarkGroup root = manager->root();
    if (root.isNull())
        return KBookmark();
    KBookmark bookmark = root.createNewSeparator();
    bookmark.setMetaDataItem("UDI", udi);
    bookmark.setMetaDataItem("isSystemItem", "true");
    return bookmark;
}

QString KFilePlacesItem::generateNewId()
{
    static int count = 0;

//    return QString::number(count++);

    return QString::number(QDateTime::currentDateTime().toTime_t())
      + '/' + QString::number(count++);


//    return QString::number(QDateTime::currentDateTime().toTime_t())
//         + '/' + QString::number(qrand());
}

void KFilePlacesItem::onAccessibilityChanged()
{
    emit itemChanged(id());
}

bool KFilePlacesItem::hasFullIcon(const KBookmark &bookmark) const
{
    return bookmark.url()==KUrl("trash:/");
}

QString KFilePlacesItem::iconNameForBookmark(const KBookmark &bookmark) const
{
    if (!m_folderIsEmpty && hasFullIcon(bookmark)) {
        return bookmark.icon()+"-full";
    } else {
        return bookmark.icon();
    }
}

void KFilePlacesItem::onListerCompleted()
{
    m_folderIsEmpty = m_lister->items().isEmpty();
    emit itemChanged(id());
}

#include "kfileplacesitem_p.moc"
