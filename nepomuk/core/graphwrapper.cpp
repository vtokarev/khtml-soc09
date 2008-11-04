/*
 * This file is part of the Nepomuk KDE project.
 * Copyright (C) 2008 Sebastian Trueg <trueg@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "graphwrapper_p.h"
#include "resourcemanager.h"

#include <Soprano/Model>
#include <Soprano/Node>
#include <Soprano/Vocabulary/RDF>
#include <Soprano/Vocabulary/NRL>
#include <Soprano/Vocabulary/NAO>

#include <QtCore/QDateTime>


Nepomuk::GraphWrapper::GraphWrapper( QObject* parent )
    : QObject( parent ),
      m_currentGraphStored( true )
{
    connect( &m_timer, SIGNAL(timeout()),
             this, SLOT(slotTimeout()) );
    m_timer.setSingleShot( true );
}


Nepomuk::GraphWrapper::~GraphWrapper()
{
}


QUrl Nepomuk::GraphWrapper::lookupCurrentGraph() const
{
    return m_currentGraph;
}


QUrl Nepomuk::GraphWrapper::currentGraph()
{
    if( !m_currentGraph.isValid() ) {
        createNewGraph();
    }

    if( !m_currentGraphStored ) {
        storeGraph( m_currentGraph );
        m_currentGraphStored = true;
    }
    return m_currentGraph;
}


void Nepomuk::GraphWrapper::slotTimeout()
{
    // generate a new graph every event loop if the last one was used
    if( m_currentGraphStored ) {
        createNewGraph();
    }
}


void Nepomuk::GraphWrapper::createNewGraph()
{
    m_currentGraph = ResourceManager::instance()->generateUniqueUri( "nepomukgraph" );
    m_currentGraphStored = false;
}


void Nepomuk::GraphWrapper::storeGraph( const QUrl& graph )
{
    Soprano::Model* model = ResourceManager::instance()->mainModel();
    QUrl metadataGraph = ResourceManager::instance()->generateUniqueUri( "nepomukgraph-metadata" );

    model->addStatement( graph,
                         Soprano::Vocabulary::RDF::type(),
                         Soprano::Vocabulary::NRL::InstanceBase(),
                         metadataGraph );
    model->addStatement( graph,
                         Soprano::Vocabulary::NAO::created(),
                         Soprano::LiteralValue( QDateTime::currentDateTime() ),
                         metadataGraph );
    model->addStatement( metadataGraph,
                         Soprano::Vocabulary::RDF::type(),
                         Soprano::Vocabulary::NRL::GraphMetadata(),
                         metadataGraph );
    model->addStatement( metadataGraph,
                         Soprano::Vocabulary::NRL::coreGraphMetadataFor(),
                         graph,
                         metadataGraph );

    //
    // We update the graph every 200 mseconds but only when entering
    // the event loop.
    // I think it is ok for the user to think that two things
    // created a quarter of a second apart are created at the same time
    //
    m_timer.start( 200 );
}

#include "graphwrapper_p.moc"
