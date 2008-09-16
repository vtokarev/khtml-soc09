/* 
 * This file is part of the Nepomuk KDE project.
 * Copyright (C) 2006-2008 Sebastian Trueg <trueg@kde.org>
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

#ifndef _NEPOMUK_RESOURCE_MANAGER_H_
#define _NEPOMUK_RESOURCE_MANAGER_H_

#include "nepomuk_export.h"

#include <QtCore/QObject>
#include <QtCore/QUrl>


namespace Soprano {
    class Model;
}

namespace Nepomuk {
    namespace Middleware {
	class Registry;
    }

    class Resource;
    class Variant;
    class ResourceManagerHelper;

    /**
     * \class ResourceManager resourcemanager.h Nepomuk/ResourceManager
     *
     * \brief The ResourceManager is the central \a %KMetaData configuration point.
     *
     * \author Sebastian Trueg <trueg@kde.org>
     */
    class NEPOMUK_EXPORT ResourceManager : public QObject
    {
        Q_OBJECT

    public:
        static ResourceManager* instance();

        /**
         * Initialize the Nepomuk framework. This method will initialize the communication with
         * the local Nepomuk-KDE services, ie. the data repository.
         *
         * When using multiple threads make sure to call this method in the main thread
         * before doing anything else.
         *
         * \return 0 if all necessary components could be found and -1 otherwise.
         *
         * FIXME: introduce error codes and human readable translated error messages.
         */
        int init();

        /**
         * \return true if init() has been called successfully, ie. the KMetaData system is connected
         * to the local RDF repository service and ready to work.
         */
        bool initialized() const;

        /**
         * Retrieve the main data storage model.
         */
        Soprano::Model* mainModel();
        
        /**
         * Override the main model used for all storage. By default the main model
         * used is the Nepomuk server main model.
         *
         * \param The model to use instead of the Nepomuk server or 0 to reset.
         *
         * \since 4.1
         */
        void setOverrideMainModel( Soprano::Model* model );

        /**
         * \deprecated Use the Resource constructor directly.
         *
         * Creates a Resource object representing the data referenced by \a uri.
         * The result is the same as from using the Resource::Resource( const QString&, const QString& )
         * constructor with an empty type.
         *
         * \return The Resource representing the data at \a uri or an invalid Resource object if the local
         * NEPOMUK RDF store does not contain an object with URI \a uri.
         */
        KDE_DEPRECATED Resource createResourceFromUri( const QString& uri );

        /**
         * Remove the resource denoted by \a uri completely.
         *
         * This method is just a wrapper around Resource::remove. The result
         * is the same.
         */
        void removeResource( const QString& uri );

        /**
         * Retrieve a list of all resources of the specified \a type.
         *
         * This includes Resources that are not synced yet so it might
         * not represent exactly the state as in the RDF store.
         */
        QList<Resource> allResourcesOfType( const QUrl& type );

        /**
         * \deprecated Use allResourcesOfType( const QString& type )
         */
        KDE_DEPRECATED QList<Resource> allResourcesOfType( const QString& type );

        /**
         * Retrieve a list of all resources that have property \a uri defined with a value of \a v.
         *
         * This includes Resources that are not synced yet so it might
         * not represent exactly the state as in the RDF store.
         *
         * \param uri The URI identifying the property. If this URI does
         *            not include a namespace the default namespace is
         *            prepended.
         * \param v The value all returned resources should have set as properts \a uri.
         */
        QList<Resource> allResourcesWithProperty( const QUrl& uri, const Variant& v );

        /**
         * \deprecated Use allResourcesWithProperty( const QString& type )
         */
        KDE_DEPRECATED QList<Resource> allResourcesWithProperty( const QString& uri, const Variant& v );

        /**
         * Generates a unique URI that is not used in the store yet. This method ca be used to 
         * generate URIs for virtual types such as Tag.
         */
        QString generateUniqueUri();

        /**
         * Generates a unique URI that is not used in the store yet. This method ca be used to 
         * generate URIs for virtual types such as Tag.
         *
         * \param label A label that the algorithm should use to try to create a more readable URI.
         *
         * \return A new unique URI which can be used to define a new resource.
         *
         * \since 4.2
         */
        QUrl generateUniqueUri( const QString& label );

        /**
         * \internal Non-public API. Used by Resource to signalize errors.
         */
        void notifyError( const QString& uri, int errorCode );

    Q_SIGNALS:
        /**
         * This signal gets emitted whenever a Resource changes due to a sync procedure.
         * Be aware that modifying resources locally via the Resource::setProperty method
         * does not result in a resourceModified signal being emitted.
         *
         * \param uri The URI of the modified resource.
         *
         * NOT IMPLEMENTED YET
         */
        void resourceModified( const QString& uri );

        /**
         * Whenever a problem occurs (like for example failed resource syncing) this 
         * signal is emitted.
         *
         * \param uri The resource related to the error.
         * \param errorCode The type of the error (Resource::ErrorCode)
         */
        void error( const QString& uri, int errorCode );

        // FIXME: add a loggin mechanism that reports successfully and failed sync operations and so on

    private Q_SLOTS:
        void slotStoreChanged();

    private:
        friend class Nepomuk::ResourceManagerHelper;
        ResourceManager();
        ~ResourceManager();

        class Private;
        Private* const d;
    };
}

#endif
