/*
 * This file is part of the KDE project.
 * Copyright (C) 2003 Carsten Pfeiffer <pfeiffer@kde.org>
 *
 * You can Freely distribute this program under the GNU Library General Public
 * License. See the file "COPYING" for the exact licensing terms.
 */

#include "kfilemetapreview.h"

#include <QtGui/QLayout>

#include <kio/previewjob.h>
#include <kpluginloader.h>
#include <kpluginfactory.h>
#include <kimagefilepreview.h>

bool KFileMetaPreview::s_tryAudioPreview = true;

KFileMetaPreview::KFileMetaPreview( QWidget *parent )
    : KPreviewWidgetBase( parent ),
      haveAudioPreview( false )
{
    QHBoxLayout *layout = new QHBoxLayout( this );
    layout->setMargin( 0 );
    layout->setSpacing( 0 );
    m_stack = new QStackedWidget( this );
    layout->addWidget( m_stack );

    // ###
//     m_previewProviders.setAutoDelete( true );
    initPreviewProviders();
}

KFileMetaPreview::~KFileMetaPreview()
{
}

void KFileMetaPreview::initPreviewProviders()
{
    qDeleteAll(m_previewProviders);
    m_previewProviders.clear();
    // hardcoded so far

    // image previews
    KImageFilePreview *imagePreview = new KImageFilePreview( m_stack );
    (void) m_stack->addWidget( imagePreview );
    m_stack->setCurrentWidget ( imagePreview );
    resize( imagePreview->sizeHint() );

    const QStringList mimeTypes = imagePreview->supportedMimeTypes();
    QStringList::ConstIterator it = mimeTypes.begin();
    for ( ; it != mimeTypes.end(); ++it )
    {
//         qDebug(".... %s", (*it).toLatin1().constData());
        m_previewProviders.insert( *it, imagePreview );
    }
}

KPreviewWidgetBase* KFileMetaPreview::findExistingProvider(const QString& mimeType, const KMimeType::Ptr& mimeInfo) const
{
    KPreviewWidgetBase* provider = m_previewProviders.value(mimeType);
    if ( provider )
        return provider;

    if ( mimeInfo ) {
        // check mime type inheritance
        const QStringList parentMimeTypes = mimeInfo->allParentMimeTypes();
        Q_FOREACH(const QString& parentMimeType, parentMimeTypes) {
            provider = m_previewProviders.value(parentMimeType);
            if (provider)
                return provider;
        }
    }

    // ### mimetype may be image/* for example, try that
    const int index = mimeType.indexOf( '/' );
    if (index > 0)
    {
        provider = m_previewProviders.value(mimeType.left(index + 1) + '*');
        if (provider)
            return provider;
    }

    return 0;
}

KPreviewWidgetBase * KFileMetaPreview::previewProviderFor( const QString& mimeType )
{
    KMimeType::Ptr mimeInfo = KMimeType::mimeType( mimeType );

    //     qDebug("### looking for: %s", mimeType.toLatin1().constData());
    // often the first highlighted item, where we can be sure, there is no plugin
    // (this "folders reflect icons" is a konq-specific thing, right?)
    if ( mimeInfo && mimeInfo->is("inode/directory") )
        return 0L;

    KPreviewWidgetBase *provider = findExistingProvider(mimeType, mimeInfo);
    if (provider)
        return provider;

//qDebug("#### didn't find anything for: %s", mimeType.toLatin1().constData());

    if ( s_tryAudioPreview &&
         !mimeType.startsWith("text/") && !mimeType.startsWith("image/") )
    {
        if ( !haveAudioPreview )
        {
            KPreviewWidgetBase *audioPreview = createAudioPreview( m_stack );
            if ( audioPreview )
            {
                haveAudioPreview = true;
                (void) m_stack->addWidget( audioPreview );
                const QStringList mimeTypes = audioPreview->supportedMimeTypes();
                QStringList::ConstIterator it = mimeTypes.begin();
                for ( ; it != mimeTypes.end(); ++it )
                {
                    // only add non already handled mimetypes
                    if ( m_previewProviders.constFind( *it ) == m_previewProviders.constEnd() )
                        m_previewProviders.insert( *it, audioPreview );
                }
            }
        }
    }

    // with the new mimetypes from the audio-preview, try again
    provider = findExistingProvider(mimeType, mimeInfo);
    if (provider)
        return provider;

    // The logic in this code duplicates the logic in PreviewJob.
    // But why do we need multiple KPreviewWidgetBase instances anyway?

    return 0L;
}

void KFileMetaPreview::showPreview(const KUrl &url)
{
    KMimeType::Ptr mt = KMimeType::findByUrl( url );
    KPreviewWidgetBase *provider = previewProviderFor( mt->name() );
    if ( provider )
    {
        if ( provider != m_stack->currentWidget() ) // stop the previous preview
            clearPreview();

        m_stack->setEnabled( true );
        m_stack->setCurrentWidget( provider );
        provider->showPreview( url );
    }
    else
    {
        clearPreview();
        m_stack->setEnabled( false );
    }
}

void KFileMetaPreview::clearPreview()
{
    if ( m_stack->currentWidget() )
        static_cast<KPreviewWidgetBase*>( m_stack->currentWidget() )->clearPreview();
}

void KFileMetaPreview::addPreviewProvider( const QString& mimeType,
                                           KPreviewWidgetBase *provider )
{
    m_previewProviders.insert( mimeType, provider );
}


void KFileMetaPreview::clearPreviewProviders()
{
	QHash<QString, KPreviewWidgetBase*>::const_iterator i = m_previewProviders.constBegin();
	while (i != m_previewProviders.constEnd())
	{
		m_stack->removeWidget(i.value());
		++i;
	}
	qDeleteAll(m_previewProviders);
    m_previewProviders.clear();
}

// static
KPreviewWidgetBase * KFileMetaPreview::createAudioPreview( QWidget *parent )
{
    KPluginFactory *factory = KPluginLoader( "kfileaudiopreview" ).factory();
    if ( !factory )
    {
        s_tryAudioPreview = false;
        return 0L;
    }
    KPreviewWidgetBase* w = factory->create<KPreviewWidgetBase>( parent );
    if ( w )
        w->setObjectName( "kfileaudiopreview" );
    return w;
}

#include "kfilemetapreview.moc"
