// -*- c-basic-offset: 3 -*-
/*  This file is part of the KDE libraries
 *  Copyright (C) 2000 Waldo Bastian <bastian@kde.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License version 2 as published by the Free Software Foundation;
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 **/

#include "kservicegroup.h"
#include "kservicegroup_p.h"
#include "kservicefactory.h"
#include "kservicegroupfactory.h"
#include "kservice.h"
#include <ksycoca.h>
#include <kglobal.h>
#include <kstandarddirs.h>
#include <klocale.h>
#include <kdebug.h>
#include <ksortablelist.h>
#include <kdesktopfile.h>
#include <kconfiggroup.h>


KServiceGroup::KServiceGroup( const QString & name )
 : KSycocaEntry(*new KServiceGroupPrivate(name))
{
}

KServiceGroup::KServiceGroup( const QString &configFile, const QString & _relpath )
 : KSycocaEntry(*new KServiceGroupPrivate(_relpath))
{
    Q_D(KServiceGroup);

    QString cfg = configFile;
    if (cfg.isEmpty())
        cfg = _relpath + ".directory";

    d->load(cfg);
}

void KServiceGroupPrivate::load(const QString &cfg)
{
  directoryEntryPath = cfg;

  const KDesktopFile desktopFile( cfg );

  const KConfigGroup config = desktopFile.desktopGroup();

  m_strCaption = config.readEntry( "Name" );
  m_strIcon = config.readEntry( "Icon" );
  m_strComment = config.readEntry( "Comment" );
  deleted = config.readEntry("Hidden", false );
  m_bNoDisplay = desktopFile.noDisplay();
  m_strBaseGroupName = config.readEntry( "X-KDE-BaseGroup" );
  suppressGenericNames = config.readEntry( "X-KDE-SuppressGenericNames", QStringList() );
//  d->sortOrder = config.readEntry("SortOrder", QStringList());

  // Fill in defaults.
  if (m_strCaption.isEmpty())
  {
     m_strCaption = path;
     if (m_strCaption.endsWith(QLatin1Char('/')))
        m_strCaption = m_strCaption.left(m_strCaption.length()-1);
     int i = m_strCaption.lastIndexOf('/');
     if (i > 0)
        m_strCaption = m_strCaption.mid(i+1);
  }
  if (m_strIcon.isEmpty())
     m_strIcon = "folder";
}

KServiceGroup::KServiceGroup( QDataStream& _str, int offset, bool deep ) :
    KSycocaEntry(*new KServiceGroupPrivate(_str, offset))
{
  Q_D(KServiceGroup);
  d->m_bDeep = deep;
  d->load( _str );
}

KServiceGroup::~KServiceGroup()
{
}

QString KServiceGroup::relPath() const
{
    return entryPath();
}

QString KServiceGroup::caption() const
{
    Q_D(const KServiceGroup);
    return d->m_strCaption;
}

QString KServiceGroup::icon() const
{
    Q_D(const KServiceGroup);
    return d->m_strIcon;
}

QString KServiceGroup::comment() const
{
    Q_D(const KServiceGroup);
    return d->m_strComment;
}

int KServiceGroup::childCount() const
{
    Q_D(const KServiceGroup);
    return d->childCount();
}

int KServiceGroupPrivate::childCount() const
{
  if (m_childCount == -1)
  {
     m_childCount = 0;

     for( KServiceGroup::List::ConstIterator it = m_serviceList.begin();
          it != m_serviceList.end(); ++it)
     {
        KSycocaEntry::Ptr p = *it;
        if (p->isType(KST_KService))
        {
            KService::Ptr service = KService::Ptr::staticCast( p );
           if (!service->noDisplay())
              m_childCount++;
        }
        else if (p->isType(KST_KServiceGroup))
        {
           KServiceGroup::Ptr serviceGroup = KServiceGroup::Ptr::staticCast( p );
           m_childCount += serviceGroup->childCount();
        }
     }
  }
  return m_childCount;
}


bool KServiceGroup::showInlineHeader() const
{
    Q_D(const KServiceGroup);
    return d->m_bShowInlineHeader;
}

bool KServiceGroup::showEmptyMenu() const
{
    Q_D(const KServiceGroup);
    return d->m_bShowEmptyMenu;
}

bool KServiceGroup::inlineAlias() const
{
    Q_D(const KServiceGroup);
    return d->m_bInlineAlias;
}

void KServiceGroup::setInlineAlias(bool _b)
{
    Q_D(KServiceGroup);
    d->m_bInlineAlias = _b;
}

void KServiceGroup::setShowEmptyMenu(bool _b)
{
    Q_D(KServiceGroup);
    d->m_bShowEmptyMenu=_b;
}

void KServiceGroup::setShowInlineHeader(bool _b)
{
    Q_D(KServiceGroup);
    d->m_bShowInlineHeader=_b;
}

int KServiceGroup::inlineValue() const
{
    Q_D(const KServiceGroup);
    return d->m_inlineValue;
}

void KServiceGroup::setInlineValue(int _val)
{
    Q_D(KServiceGroup);
    d->m_inlineValue = _val;
}

bool KServiceGroup::allowInline() const
{
    Q_D(const KServiceGroup);
    return d->m_bAllowInline;
}

void KServiceGroup::setAllowInline(bool _b)
{
    Q_D(KServiceGroup);
    d->m_bAllowInline = _b;
}

bool KServiceGroup::noDisplay() const
{
    Q_D(const KServiceGroup);
  return d->m_bNoDisplay || d->m_strCaption.startsWith('.');
}

QStringList KServiceGroup::suppressGenericNames() const
{
    Q_D(const KServiceGroup);
  return d->suppressGenericNames;
}

void KServiceGroupPrivate::load( QDataStream& s )
{
  QStringList groupList;
  qint8 noDisplay;
  qint8 _showEmptyMenu;
  qint8 inlineHeader;
  qint8 _inlineAlias;
  qint8 _allowInline;
  s >> m_strCaption >> m_strIcon >>
      m_strComment >> groupList >> m_strBaseGroupName >> m_childCount >>
      noDisplay >> suppressGenericNames >> directoryEntryPath >>
      sortOrder >> _showEmptyMenu >> inlineHeader >> _inlineAlias >> _allowInline;

  m_bNoDisplay = (noDisplay != 0);
  m_bShowEmptyMenu = ( _showEmptyMenu != 0 );
  m_bShowInlineHeader = ( inlineHeader != 0 );
  m_bInlineAlias = ( _inlineAlias != 0 );
  m_bAllowInline = ( _allowInline != 0 );

  if (m_bDeep)
  {
     for(QStringList::ConstIterator it = groupList.begin();
         it != groupList.end(); ++it)
     {
        QString path = *it;
        if (path[path.length()-1] == '/')
        {
           KServiceGroup::Ptr serviceGroup;
           serviceGroup = KServiceGroupFactory::self()->findGroupByDesktopPath(path, false);
           if (serviceGroup)
               m_serviceList.append( KServiceGroup::SPtr::staticCast(serviceGroup) );
        }
        else
        {
           KService::Ptr service;
           service = KServiceFactory::self()->findServiceByDesktopPath(path);
           if (service)
              m_serviceList.append( KServiceGroup::SPtr::staticCast(service) );
        }
     }
  }
}

void KServiceGroup::addEntry( const KSycocaEntry::Ptr& entry)
{
    Q_D(KServiceGroup);
  d->m_serviceList.append(entry);
}

void KServiceGroupPrivate::save( QDataStream& s )
{
  KSycocaEntryPrivate::save( s );

  QStringList groupList;
  for( KServiceGroup::List::ConstIterator it = m_serviceList.begin();
       it != m_serviceList.end(); ++it)
  {
     KSycocaEntry::Ptr p = *it;
     if (p->isType(KST_KService))
     {
        KService::Ptr service = KService::Ptr::staticCast( p );
        groupList.append( service->entryPath() );
     }
     else if (p->isType(KST_KServiceGroup))
     {
        KServiceGroup::Ptr serviceGroup = KServiceGroup::Ptr::staticCast( p );
        groupList.append( serviceGroup->relPath() );
     }
     else
     {
        //fprintf(stderr, "KServiceGroup: Unexpected object in list!\n");
     }
  }

  (void) childCount();

  qint8 noDisplay = m_bNoDisplay ? 1 : 0;
  qint8 _showEmptyMenu = m_bShowEmptyMenu ? 1 : 0;
  qint8 inlineHeader = m_bShowInlineHeader ? 1 : 0;
  qint8 _inlineAlias = m_bInlineAlias ? 1 : 0;
  qint8 _allowInline = m_bAllowInline ? 1 : 0;
  s << m_strCaption << m_strIcon <<
      m_strComment << groupList << m_strBaseGroupName << m_childCount <<
      noDisplay << suppressGenericNames << directoryEntryPath <<
      sortOrder <<_showEmptyMenu <<inlineHeader<<_inlineAlias<<_allowInline;
}

QList<KServiceGroup::Ptr> KServiceGroup::groupEntries(EntriesOptions options)
{
    Q_D(KServiceGroup);
    bool sort = options & SortEntries || options & AllowSeparators;
    QList<KServiceGroup::Ptr> list;
    List tmp = d->entries(this, sort, options & ExcludeNoDisplay, options & AllowSeparators, options & SortByGenericName);
    foreach(const SPtr &ptr, tmp) {
        if (ptr->isType(KST_KServiceGroup))
            list.append(Ptr::staticCast(ptr));
        else if (ptr->isType(KST_KServiceSeparator))
            list.append(KServiceGroup::Ptr(static_cast<KServiceGroup *>(new KSycocaEntry())));
        else if (sort && ptr->isType(KST_KService))
            break;
    }
    return list;
}

KService::List KServiceGroup::serviceEntries(EntriesOptions options)
{
    Q_D(KServiceGroup);
    bool sort = options & SortEntries || options & AllowSeparators;
    QList<KService::Ptr> list;
    List tmp = d->entries(this, sort, options & ExcludeNoDisplay, options & AllowSeparators, options & SortByGenericName);
    bool foundService = false;
    foreach(const SPtr &ptr, tmp) {
        if (ptr->isType(KST_KService)) {
            list.append(KService::Ptr::staticCast(ptr));
            foundService = true;
        }
        else if (ptr->isType(KST_KServiceSeparator) && foundService) {
            list.append(KService::Ptr(static_cast<KService *>(new KSycocaEntry())));
        }
    }
    return list;
}

KServiceGroup::List
KServiceGroup::entries(bool sort)
{
    Q_D(KServiceGroup);
    return d->entries(this, sort, true, false, false);
}

KServiceGroup::List
KServiceGroup::entries(bool sort, bool excludeNoDisplay)
{
    Q_D(KServiceGroup);
    return d->entries(this, sort, excludeNoDisplay, false, false);
}

KServiceGroup::List
KServiceGroup::entries(bool sort, bool excludeNoDisplay, bool allowSeparators, bool sortByGenericName)
{
    Q_D(KServiceGroup);
    return d->entries(this, sort, excludeNoDisplay, allowSeparators, sortByGenericName);
}

static void addItem(KServiceGroup::List &sorted, const KSycocaEntry::Ptr &p, bool &addSeparator)
{
   if (addSeparator && !sorted.isEmpty())
      sorted.append(KSycocaEntry::Ptr(new KServiceSeparator()));
   sorted.append(p);
   addSeparator = false;
}

KServiceGroup::List
KServiceGroupPrivate::entries(KServiceGroup *group, bool sort, bool excludeNoDisplay, bool allowSeparators, bool sortByGenericName)
{
    KServiceGroup::Ptr grp;

    // If the entries haven't been loaded yet, we have to reload ourselves
    // together with the entries. We can't only load the entries afterwards
    // since the offsets could have been changed if the database has changed.

    if (!m_bDeep) {

        grp = KServiceGroupFactory::self()->findGroupByDesktopPath(path, true);

        group = grp.data();
        if (0 == group) // No guarantee that we still exist!
            return KServiceGroup::List();
    }

    if (!sort)
        return group->d_func()->m_serviceList;

    // Sort the list alphabetically, according to locale.
    // Groups come first, then services.

    KSortableList<KServiceGroup::SPtr,QByteArray> slist;
    KSortableList<KServiceGroup::SPtr,QByteArray> glist;
    for (KServiceGroup::List::ConstIterator it(group->d_func()->m_serviceList.begin()); it != group->d_func()->m_serviceList.end(); ++it)
    {
        KSycocaEntry::Ptr p = (*it);
        bool noDisplay = p->isType(KST_KServiceGroup) ?
                                   static_cast<KServiceGroup *>(p.data())->noDisplay() :
                                   static_cast<KService *>(p.data())->noDisplay();
        if (excludeNoDisplay && noDisplay)
           continue;
        // Choose the right list
        KSortableList<KServiceGroup::SPtr,QByteArray> & list = p->isType(KST_KServiceGroup) ? glist : slist;
        QString name;
        if (p->isType(KST_KServiceGroup))
          name = static_cast<KServiceGroup *>(p.data())->caption();
        else if (sortByGenericName)
          name = static_cast<KService *>(p.data())->genericName() + ' ' + p->name();
        else
          name = p->name() + ' ' + static_cast<KService *>(p.data())->genericName();

        QByteArray key;
        // strxfrm() crashes on Solaris
#ifndef USE_SOLARIS
        // maybe it'd be better to use wcsxfrm() where available
        key.resize( name.length() * 4 + 1 );
        size_t ln = strxfrm( key.data(), name.toLocal8Bit().data(), key.size());
        if( ln != size_t( -1 ))
        {
            if( (int)ln >= key.size())
            { // didn't fit?
                key.resize( ln + 1 );
                if( strxfrm( key.data(), name.toLocal8Bit().data(), key.size()) == size_t( -1 ))
                    key = name.toLocal8Bit();
            }
        }
        else
#endif
        {
            key = name.toLocal8Bit();
        }
        list.insert(key,KServiceGroup::SPtr(*it));
    }
    // Now sort
    slist.sort();
    glist.sort();

    if (sortOrder.isEmpty())
    {
       sortOrder << ":M";
       sortOrder << ":F";
       sortOrder << ":OIH IL[4]"; //just inline header
    }

    QString rp = path;
    if(rp == "/") rp.clear();

    // Iterate through the sort spec list.
    // If an entry gets mentioned explicitly, we remove it from the sorted list
    for (QStringList::ConstIterator it(sortOrder.begin()); it != sortOrder.end(); ++it)
    {
        const QString &item = *it;
        if (item.isEmpty()) continue;
        if (item[0] == '/')
        {
          QString groupPath = rp + item.mid(1) + '/';
           // Remove entry from sorted list of services.
          for(KSortableList<KServiceGroup::SPtr,QByteArray>::Iterator it2 = glist.begin(); it2 != glist.end(); ++it2)
          {
             const KServiceGroup::Ptr group = KServiceGroup::Ptr::staticCast( (*it2).value() );
             if (group->relPath() == groupPath)
             {
                glist.erase(it2);
                break;
             }
          }
        }
        else if (item[0] != ':')
        {
           // Remove entry from sorted list of services.
           // TODO: Remove item from sortOrder-list if not found
           // TODO: This prevents duplicates
          for(KSortableList<KServiceGroup::SPtr,QByteArray>::Iterator it2 = slist.begin(); it2 != slist.end(); ++it2)
          {
             const KService::Ptr service = KService::Ptr::staticCast( (*it2).value() );
             if (service->menuId() == item)
             {
                slist.erase(it2);
                break;
             }
          }
        }
    }

    KServiceGroup::List sorted;

    bool needSeparator = false;
    // Iterate through the sort spec list.
    // Add the entries to the list according to the sort spec.
    for (QStringList::ConstIterator it(sortOrder.begin()); it != sortOrder.end(); ++it)
    {
        const QString &item = *it;
        if (item.isEmpty()) continue;
        if (item[0] == ':')
        {
          // Special condition...
          if (item == ":S")
          {
             if (allowSeparators)
                needSeparator = true;
          }
          else if ( item.contains( ":O" ) )
          {
              //todo parse attribute:
              QString tmp(  item );
              tmp = tmp.remove(":O");
              QStringList optionAttribute = tmp.split(' ', QString::SkipEmptyParts);
              if ( optionAttribute.isEmpty() )
                  optionAttribute.append( tmp );
              bool showEmptyMenu = false;
              bool showInline = false;
              bool showInlineHeader = false;
              bool showInlineAlias = false;
              int inlineValue = -1;

              for ( QStringList::Iterator it3 = optionAttribute.begin(); it3 != optionAttribute.end(); ++it3 )
              {
                  parseAttribute( *it3,  showEmptyMenu, showInline, showInlineHeader, showInlineAlias, inlineValue );
              }
              for(KSortableList<KServiceGroup::SPtr,QByteArray>::Iterator it2 = glist.begin(); it2 != glist.end(); ++it2)
              {
                  KServiceGroup::Ptr group = KServiceGroup::Ptr::staticCast( (*it2).value() );
                  group->setShowEmptyMenu(  showEmptyMenu  );
                  group->setAllowInline( showInline );
                  group->setShowInlineHeader( showInlineHeader );
                  group->setInlineAlias( showInlineAlias );
                  group->setInlineValue( inlineValue );
              }

          }
          else if (item == ":M")
          {
            // Add sorted list of sub-menus
            for(KSortableList<KServiceGroup::SPtr,QByteArray>::const_iterator it2 = glist.begin(); it2 != glist.end(); ++it2)
            {
              addItem(sorted, (*it2).value(), needSeparator);
            }
          }
          else if (item == ":F")
          {
            // Add sorted list of services
            for(KSortableList<KServiceGroup::SPtr,QByteArray>::const_iterator it2 = slist.begin(); it2 != slist.end(); ++it2)
            {
              addItem(sorted, (*it2).value(), needSeparator);
            }
          }
          else if (item == ":A")
          {
            // Add sorted lists of services and submenus
            KSortableList<KServiceGroup::SPtr,QByteArray>::Iterator it_s = slist.begin();
            KSortableList<KServiceGroup::SPtr,QByteArray>::Iterator it_g = glist.begin();

            while(true)
            {
               if (it_s == slist.end())
               {
                  if (it_g == glist.end())
                     break; // Done

                  // Insert remaining sub-menu
                  addItem(sorted, (*it_g).value(), needSeparator);
                  it_g++;
               }
               else if (it_g == glist.end())
               {
                  // Insert remaining service
                  addItem(sorted, (*it_s).value(), needSeparator);
                  it_s++;
               }
               else if ((*it_g).key() < (*it_s).key())
               {
                  // Insert sub-menu first
                  addItem(sorted, (*it_g).value(), needSeparator);
                  it_g++;
               }
               else
               {
                  // Insert service first
                  addItem(sorted, (*it_s).value(), needSeparator);
                  it_s++;
               }
            }
          }
        }
        else if (item[0] == '/')
        {
            QString groupPath = rp + item.mid(1) + '/';

            for (KServiceGroup::List::ConstIterator it2(group->d_func()->m_serviceList.begin()); it2 != group->d_func()->m_serviceList.end(); ++it2)
            {
                if (!(*it2)->isType(KST_KServiceGroup))
                    continue;
                KServiceGroup::Ptr group = KServiceGroup::Ptr::staticCast( *it2 );
                if (group->relPath() == groupPath)
                {
                    if (!excludeNoDisplay || !group->noDisplay())
                    {
                        ++it;
                        const QString &nextItem =
                            (it == sortOrder.end()) ? QString() : *it;

                        if ( nextItem.startsWith( ":O" ) )
                        {
                            QString tmp( nextItem );
                            tmp = tmp.remove(":O");
                            QStringList optionAttribute = tmp.split(' ', QString::SkipEmptyParts);
                            if ( optionAttribute.isEmpty() )
                                optionAttribute.append( tmp );
                            bool bShowEmptyMenu = false;
                            bool bShowInline = false;
                            bool bShowInlineHeader = false;
                            bool bShowInlineAlias = false;
                            int inlineValue = -1;
                            for ( QStringList::Iterator it3 = optionAttribute.begin(); it3 != optionAttribute.end(); ++it3 )
                            {
                                parseAttribute( *it3 , bShowEmptyMenu, bShowInline, bShowInlineHeader, bShowInlineAlias , inlineValue );
                                group->setShowEmptyMenu( bShowEmptyMenu );
                                group->setAllowInline( bShowInline );
                                group->setShowInlineHeader( bShowInlineHeader );
                                group->setInlineAlias( bShowInlineAlias );
                                group->setInlineValue( inlineValue );
                            }
                        }
                        else
                            it--;

                        addItem(sorted, KServiceGroup::SPtr::staticCast(group), needSeparator);
                    }
                    break;
                }
            }
        }
        else
        {
            for (KServiceGroup::List::ConstIterator it2(group->d_func()->m_serviceList.begin()); it2 != group->d_func()->m_serviceList.end(); ++it2)
            {
                if (!(*it2)->isType(KST_KService))
                    continue;
                const KService::Ptr service = KService::Ptr::staticCast( *it2 );
                if (service->menuId() == item)
                {
                    if (!excludeNoDisplay || !service->noDisplay())
                        addItem(sorted, (*it2), needSeparator);
                    break;
                }
            }
        }
    }

    return sorted;
}

void KServiceGroupPrivate::parseAttribute( const QString &item ,  bool &showEmptyMenu, bool &showInline, bool &showInlineHeader, bool & showInlineAlias , int &inlineValue )
{
    if( item == "ME") //menu empty
        showEmptyMenu=true;
    else if ( item == "NME") //not menu empty
        showEmptyMenu=false;
    else if( item == "I") //inline menu !
        showInline = true;
    else if ( item == "NI") //not inline menu!
        showInline = false;
    else if( item == "IH") //inline  header!
        showInlineHeader= true;
    else if ( item == "NIH") //not inline  header!
        showInlineHeader = false;
    else if( item == "IA") //inline alias!
        showInlineAlias = true;
    else if (  item == "NIA") //not inline alias!
        showInlineAlias = false;
    else if( ( item ).contains( "IL" )) //inline limite!
    {
        QString tmp( item );
        tmp = tmp.remove( "IL[" );
        tmp = tmp.remove( ']' );
        bool ok;
        int _inlineValue = tmp.toInt(&ok);
        if ( !ok ) //error
            _inlineValue = -1;
        inlineValue =  _inlineValue;
    }
    else
        kDebug()<<" This attribute is not supported :"<<item;
}

void KServiceGroup::setLayoutInfo(const QStringList &layout)
{
    Q_D(KServiceGroup);
    d->sortOrder = layout;
}

QStringList KServiceGroup::layoutInfo() const
{
    Q_D(const KServiceGroup);
    return d->sortOrder;
}

KServiceGroup::Ptr
KServiceGroup::baseGroup( const QString & _baseGroupName )
{
    return KServiceGroupFactory::self()->findBaseGroup(_baseGroupName, true);
}

KServiceGroup::Ptr
KServiceGroup::root()
{
   return KServiceGroupFactory::self()->findGroupByDesktopPath("/", true);
}

KServiceGroup::Ptr
KServiceGroup::group(const QString &relPath)
{
   if (relPath.isEmpty()) return root();
   return KServiceGroupFactory::self()->findGroupByDesktopPath(relPath, true);
}

KServiceGroup::Ptr
KServiceGroup::childGroup(const QString &parent)
{
   return KServiceGroupFactory::self()->findGroupByDesktopPath("#parent#"+parent, true);
}

QString KServiceGroup::baseGroupName() const
{
    return d_func()->m_strBaseGroupName;
}

QString
KServiceGroup::directoryEntryPath() const
{
    Q_D(const KServiceGroup);
   return d->directoryEntryPath;
}

class KServiceSeparatorPrivate : public KSycocaEntryPrivate
{
    public:
        K_SYCOCATYPE( KST_KServiceSeparator, KSycocaEntryPrivate )

        KServiceSeparatorPrivate(const QString &name)
            : KSycocaEntryPrivate(name)
        {
        }

        virtual QString name() const
        {
            return QLatin1String("separator");
        }

};

KServiceSeparator::KServiceSeparator( )
    : KSycocaEntry(*new KServiceSeparatorPrivate("separator"))
{
}


