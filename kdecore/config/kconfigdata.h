/*
   This file is part of the KDE libraries
   Copyright (c) 2006, 2007 Thomas Braxton <kde.braxton@gmail.com>
   Copyright (c) 1999-2000 Preston Brown <pbrown@kde.org>
   Copyright (C) 1996-2000 Matthias Kalle Dalheimer <kalle@kde.org>

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

#ifndef KCONFIGDATA_H
#define KCONFIGDATA_H

#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <QtCore/QMap>
#include <QtCore/QDebug>

/**
 * map/dict/list config node entry.
 * @internal
 */
struct KEntry
{
  /** Constructor. @internal */
  KEntry()
    : mValue(), bDirty(false),
      bGlobal(false), bImmutable(false), bDeleted(false), bExpand(false) {}
  /** @internal */
  QByteArray mValue;
  /**
   * Must the entry be written back to disk?
   */
  bool    bDirty :1;
  /**
   * Entry should be written to the global config file
   */
  bool    bGlobal:1;
  /**
   * Entry can not be modified.
   */
  bool    bImmutable:1;
  /**
   * Entry has been deleted.
   */
  bool    bDeleted:1;
  /**
   * Whether to apply dollar expansion or not.
   */
  bool    bExpand:1;
};


inline bool operator ==(const KEntry &k1, const KEntry &k2)
{
    int result = qstrcmp(k1.mValue.constData(), k2.mValue.constData());
    return (result == 0 && k1.bDirty == k2.bDirty && k1.bGlobal == k2.bGlobal
        && k1.bImmutable == k2.bImmutable && k1.bDeleted == k2.bDeleted &&
        k1.bExpand == k2.bExpand);
}

inline bool operator !=(const KEntry &k1, const KEntry &k2)
{
    return !(k1 == k2);
}

/**
 * key structure holding both the actual key and the group
 * to which it belongs.
 * @internal
 */
struct KEntryKey
{
  /** Constructor. @internal */
  KEntryKey(const QByteArray& _group = QByteArray(),
	    const QByteArray& _key = QByteArray(), bool isLocalized=false, bool isDefault=false)
      : mGroup(_group), mKey(_key), bLocal(isLocalized), bDefault(isDefault), bRaw(false)
      { ; }
  /**
   * The "group" to which this EntryKey belongs
   */
  QByteArray mGroup;
  /**
   * The _actual_ key of the entry in question
   */
  QByteArray mKey;
  /**
   * Entry is localised or not
   */
  bool    bLocal  :1;
  /**
   * Entry indicates if this is a default value.
   */
  bool    bDefault:1;
  /** @internal
   * Key is a raw unprocessed key.
   * @warning this should only be set during merging, never for normal use.
   */
  bool    bRaw:1;
};

/**
 * Compares two KEntryKeys (needed for QMap). The order is localized, localized-default,
 * non-localized, non-localized-default
 * @internal
 */
inline bool operator <(const KEntryKey &k1, const KEntryKey &k2)
{
    int result = qstrcmp(k1.mGroup.constData(), k2.mGroup.constData());
    if (result != 0) {
        return result < 0;
    }

    result = qstrcmp(k1.mKey.constData(), k2.mKey.constData());
    if (result != 0) {
        return result < 0;
    }

    if (k1.bLocal != k2.bLocal)
        return k1.bLocal;
    return (!k1.bDefault && k2.bDefault);
}

/**
 * \relates KEntry
 * type specifying a map of entries (key,value pairs).
 * The keys are actually a key in a particular config file group together
 * with the group name.
 * @internal
 */
class KEntryMap : public QMap<KEntryKey, KEntry>
{
    public:
        enum SearchFlag {
            SearchDefaults=1,
            SearchLocalized=2
        };
        Q_DECLARE_FLAGS(SearchFlags, SearchFlag)

        enum EntryOption {
            EntryDirty=1,
            EntryGlobal=2,
            EntryImmutable=4,
            EntryDeleted=8,
            EntryExpansion=16,
            EntryRawKey=32,
            EntryDefault=(SearchDefaults<<16),
            EntryLocalized=(SearchLocalized<<16)
        };
        Q_DECLARE_FLAGS(EntryOptions, EntryOption)

        Iterator findExactEntry(const QByteArray& group, const QByteArray& key = QByteArray(),
                           SearchFlags flags = SearchFlags())
        {
            KEntryKey theKey(group, key, false, bool(flags&SearchDefaults));

            // try the localized key first
            if (flags&SearchLocalized) {
                theKey.bLocal = true;
                return find(theKey);
            }
            return find(theKey);
        }

        Iterator findEntry(const QByteArray& group, const QByteArray& key = QByteArray(),
                           SearchFlags flags = SearchFlags())
        {
            KEntryKey theKey(group, key, false, bool(flags&SearchDefaults));

            // try the localized key first
            if (flags&SearchLocalized) {
                theKey.bLocal = true;

                Iterator it = find(theKey);
                if (it != end())
                    return it;

                theKey.bLocal = false;
            }
            return find(theKey);
        }

        ConstIterator findEntry(const QByteArray& group, const QByteArray& key = QByteArray(),
                                SearchFlags flags = SearchFlags()) const
        {
            KEntryKey theKey(group, key, false, bool(flags&SearchDefaults));

            // try the localized key first
            if (flags&SearchLocalized) {
                theKey.bLocal = true;

                ConstIterator it = find(theKey);
                if (it != constEnd())
                    return it;

                theKey.bLocal = false;
            }
            return find(theKey);
        }
        
        /**
         * Returns true if the entry gets dirtied or false in other case
         */
        bool setEntry(const QByteArray& group, const QByteArray& key,
                      const QByteArray& value, EntryOptions options)
        {
            KEntryKey k;
            KEntry e;
            bool newKey = false;

            const Iterator it = findExactEntry(group, key, SearchFlags(options>>16));
            
            if (key.isEmpty()) { // inserting a group marker
                k.mGroup = group;
                e.bImmutable = (options&EntryImmutable);
                if(it == end())
                {
                    insert(k, e);
                    return true;
                } else if(it.value() == e)
                    return false;
                
                it.value() = e;
                return true;
            }


            if (it != end()) {
                if (it->bImmutable)
                    return false; // we cannot change this entry. Inherits group immutability.
                k = it.key();
                e = *it;
            } else {
                // make sure the group marker is in the map
                ConstIterator cit = findEntry(group);
                if (cit == constEnd())
                    insert(KEntryKey(group), KEntry());
                else if (cit->bImmutable)
                    return false; // this group is immutable, so we cannot change this entry.

                k = KEntryKey(group, key);
                newKey = true;
            }

            // set these here, since we may be changing the type of key from the one we found
            k.bLocal = (options&EntryLocalized);
            k.bDefault = (options&EntryDefault);
            k.bRaw = (options&EntryRawKey);

/*            qDebug() << "changing" << QString("[%1,%2]").arg(group).arg(key).toLatin1().constData()
                    << '=' << maybeNull(e.mValue)
                    << entryDataToQString(e).toLatin1().constData();*/
            e.mValue = value;
            e.bDirty = e.bDirty || (options&EntryDirty);
            e.bGlobal = (options&EntryGlobal);  //we can't use || here, because changes to entries in
                                                //kdeglobals would be written to kdeglobals instead
                                                //of the local config file, regardless of the globals flag
            e.bImmutable = e.bImmutable || (options&EntryImmutable);
            if (value.isNull())
                e.bDeleted = e.bDeleted || (options&EntryDeleted);
            else
                e.bDeleted = false; // setting a value to a previously deleted entry
            e.bExpand = (options&EntryExpansion);

//             qDebug() << "to" << QString("[%1,%2]").arg(group).arg(key).toLatin1().constData()
//                     << '=' << maybeNull(e.mValue)
//                     << entryDataToQString(e).toLatin1().constData();
            if(newKey)
            {
                insert(k, e);
                if(k.bDefault)
                {
                    k.bDefault = false;
                    insert(k, e);
                }
                // TODO check for presence of unlocalized key
                return true;
            } else {
//                KEntry e2 = it.value();
                if(it.value() != e)
                {
                    it.value() = e;
                    if(k.bDefault)
                    {
                        k.bDefault = false;
                        insert(k, e);
                    }
                    if (!(options & EntryLocalized)) {
                        KEntryKey theKey(group, key, true, false);
                        remove(theKey);
                        if (k.bDefault) {
                            theKey.bDefault = false;
                            remove(theKey);
                        }
                    }
                    return true;
                } else {
                    if (!(options & EntryLocalized)) {
                        KEntryKey theKey(group, key, true, false);
                        bool ret = false;
                        Iterator cit = find(theKey);
                        if (cit != end()) {
                            erase(cit);
                            ret = true;
                        }
                        if (k.bDefault) {
                            theKey.bDefault = false;
                            Iterator cit = find(theKey);
                            if (cit != end()) {
                                erase(cit);
                                return true;
                            }
                        }
                        return ret;
                    }
                    // When we are writing a default, we know that the non-
                    // default is the same as the default, so we can simply
                    // use the same branch.
                    return false;
                }
            }
        }
        
        void setEntry(const QByteArray& group, const QByteArray& key,
                      const QString & value, EntryOptions options)
        {
            setEntry(group, key, value.toUtf8(), options);
        }

        QString getEntry(const QByteArray& group, const QByteArray& key,
                         const QString & defaultValue = QString(),
                         SearchFlags flags = SearchFlags(),
                         bool * expand=0) const
        {
            const ConstIterator it = findEntry(group, key, flags);
            QString theValue = defaultValue;

            if (it != constEnd() && !it->bDeleted) {
                if (!it->mValue.isNull()) {
                    const QByteArray data=it->mValue;
                    theValue = QString::fromUtf8(data.constData(), data.length());
                    if (expand)
                        *expand = it->bExpand;
                }
            }

            return theValue;
        }
        
        

        bool hasEntry(const QByteArray& group, const QByteArray& key = QByteArray(),
                      SearchFlags flags = SearchFlags()) const
        {
            const ConstIterator it = findEntry(group, key, flags);
            if (it == constEnd())
                return false;
            if (key.isNull()) // looking for group marker
                return it->mValue.isNull();
            return !it->bDeleted;
        }

        bool getEntryOption(const ConstIterator& it, EntryOption option) const
        {
            if (it != constEnd()) {
                switch (option) {
                    case EntryDirty:
                        return it->bDirty;
                    case EntryLocalized:
                        return it.key().bLocal;
                    case EntryGlobal:
                        return it->bGlobal;
                    case EntryImmutable:
                        return it->bImmutable;
                    case EntryDeleted:
                        return it->bDeleted;
                    case EntryExpansion:
                        return it->bExpand;
                    default:
                        break; // fall through
                }
            }

            return false;
        }
        bool getEntryOption(const QByteArray& group, const QByteArray& key,
                            SearchFlags flags, EntryOption option) const
        {
            return getEntryOption(findEntry(group, key, flags), option);
        }

        void setEntryOption(Iterator it, EntryOption option, bool bf)
        {
            if (it != constEnd()) {
                switch (option) {
                    case EntryDirty:
                        it->bDirty = bf;
                        break;
                    case EntryGlobal:
                        it->bGlobal = bf;
                        break;
                    case EntryImmutable:
                        it->bImmutable = bf;
                        break;
                    case EntryDeleted:
                        it->bDeleted = bf;
                        break;
                    case EntryExpansion:
                        it->bExpand = bf;
                        break;
                    default:
                        break; // fall through
                }
            }
        }
        void setEntryOption(const QByteArray& group, const QByteArray& key, SearchFlags flags,
                            EntryOption option, bool bf)
        {
            setEntryOption(findEntry(group, key, flags), option, bf);
        }

        void revertEntry(const QByteArray& group, const QByteArray& key, SearchFlags flags=SearchFlags())
        {
            Iterator entry = findEntry(group, key, flags);
            if (entry != constEnd()) {
/*                qDebug() << "reverting" << QString("[%1,%2]").arg(group).arg(key).toLatin1().constData()
                        << '=' << entry->mValue
                        << entryDataToQString(*entry).toLatin1().constData();*/
                const ConstIterator defaultEntry(entry+1);
                if (defaultEntry != constEnd() && defaultEntry.key().bDefault) {
                    *entry = *defaultEntry;
                    entry->bDirty = true;
                } else if (!entry->mValue.isNull()){
                    entry->mValue = QByteArray();
                    entry->bDirty = true;
                    entry->bDeleted = true;
                }
/*                qDebug() << "to" << QString("[%1,%2]").arg(group).arg(key).toLatin1().constData()
                        << '=' << entry->mValue
                        << entryDataToQString(*entry).toLatin1().constData();*/
            }
        }
};
Q_DECLARE_OPERATORS_FOR_FLAGS(KEntryMap::SearchFlags)
Q_DECLARE_OPERATORS_FOR_FLAGS(KEntryMap::EntryOptions)

/**
 * \relates KEntry
 * type for iterating over keys in a KEntryMap in sorted order.
 * @internal
 */
typedef QMap<KEntryKey, KEntry>::Iterator KEntryMapIterator;

/**
 * \relates KEntry
 * type for iterating over keys in a KEntryMap in sorted order.
 * It is const, thus you cannot change the entries in the iterator,
 * only examine them.
 * @internal
 */
typedef QMap<KEntryKey, KEntry>::ConstIterator KEntryMapConstIterator;

#endif
