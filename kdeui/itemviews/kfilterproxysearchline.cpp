/* This file is part of the KDE project
   Copyright (C) 2007 Omat Holding B.V. <info@omat.nl>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include "kfilterproxysearchline.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QTimer>
#include <QSortFilterProxyModel>

#include <klineedit.h>
#include <klocale.h>
#include <kdebug.h>

/**
 * Private class that helps to provide binary compatibility between releases.
 * @internal
 */
//@cond PRIVATE
class KFilterProxySearchLine::Private : public QObject {
public:
    Private( KFilterProxySearchLine* parent) : QObject( parent ),
                                q(parent), proxy(0), searchLine(0) {}
    KFilterProxySearchLine* q;
    QSortFilterProxyModel* proxy;
    KLineEdit* searchLine;

    void slotSearchLineChange( const QString& newText );
    void slotSearchLineActivate();
};

void KFilterProxySearchLine::Private::slotSearchLineChange( const QString& )
{
    static QTimer* timer = new QTimer( this );
    static bool first = true;
    if ( first ) {
        timer->setSingleShot( true );
        connect( timer, SIGNAL( timeout() ), q, SLOT( slotSearchLineActivate() ) );
        first=false;
    }
    timer->start( 300 );
}

void KFilterProxySearchLine::Private::slotSearchLineActivate()
{
    if ( !proxy )
        return;

    proxy->setFilterKeyColumn( -1 );
    proxy->setFilterCaseSensitivity( Qt::CaseInsensitive );
    proxy->setFilterRegExp( searchLine->text() );
    kDebug() << "Setting filter to " << searchLine->text();
}
//@endcond

KFilterProxySearchLine::KFilterProxySearchLine( QWidget* parent )
        : QWidget( parent ), d( new Private( this ) )
{
    QLabel *label = new QLabel( i18n( "S&earch:" ), this );
    label->setObjectName( QLatin1String( "kde toolbar widget" ) );

    d->searchLine = new KLineEdit( this );
    d->searchLine->setClearButtonShown( true );

    label->setBuddy( d->searchLine );
    label->show();

    QHBoxLayout* layout = new QHBoxLayout( this );
    layout->setSpacing( 5 );
    layout->setMargin( 0 );
    layout->addWidget( label );
    layout->addWidget( d->searchLine );

    connect( d->searchLine, SIGNAL( textChanged( const QString& ) ),
             SLOT( slotSearchLineChange( const QString& ) ) );
}

KFilterProxySearchLine::~KFilterProxySearchLine()
{
    delete d;
}

void KFilterProxySearchLine::update()
{
    if ( d->searchLine->text().isEmpty() )
        return;

    d->slotSearchLineActivate();
}

void KFilterProxySearchLine::setText( const QString& text )
{
    d->searchLine->setText( text );
}

void KFilterProxySearchLine::setProxy( QSortFilterProxyModel* proxy ) {
    d->proxy = proxy;
}

#include "kfilterproxysearchline.moc"
