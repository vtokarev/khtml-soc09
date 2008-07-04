/* This file is part of the KDE libraries
   Copyright (C) 2005 David Faure <faure@kde.org>

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

#ifndef KHBOX_H
#define KHBOX_H

#include <kdeui_export.h>

#include <QtGui/QFrame>

class QChildEvent;

/**
 * A container widget which arranges its children horizontally.
 * When using a KHBox you don't need to create a layout nor
 * to add the child widgets to it.
 *
 * @see KVBox
 */
class KDEUI_EXPORT KHBox : public QFrame
{
  Q_OBJECT
  
  public:
    /**
     * Creates a new hbox.
     */
    explicit KHBox( QWidget* parent = 0 );
  
    /**
     * Destructor.
     */
    ~KHBox();

    /**
     * Sets the @p margin of the hbox.
     */
    void setMargin( int margin );

    /**
     * Sets the spacing between the child widgets to @p space
     */
    void setSpacing( int space );

    /*
     * Sets the stretch factor of @p widget to @p stretch.
     */
    void setStretchFactor( QWidget* widget, int stretch );

    virtual QSize sizeHint() const;
    virtual QSize minimumSizeHint() const;

  protected:
    /*
     * @internal
     */
    KHBox( bool vertical, QWidget* parent );

    virtual void childEvent( QChildEvent* ev );

  private:
    class Private;
    friend class Private;
    Private* const d;
    
    Q_DISABLE_COPY(KHBox)
};

#endif
