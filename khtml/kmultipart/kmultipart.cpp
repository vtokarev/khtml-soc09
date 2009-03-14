/* This file is part of the KDE project
   Copyright (C) 2002 David Faure <david@mandrakesoft.com>

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

#include "kmultipart.h"


#include <kcomponentdata.h>
#include <kmimetype.h>
#include <klocale.h>
#include <kjobuidelegate.h>
#include <kio/job.h>
#include <QtCore/QFile>
#include <ktemporaryfile.h>
#include <kmessagebox.h>
#include <kparts/componentfactory.h>
#include <kparts/genericfactory.h>
#include <khtml_part.h>
#include <unistd.h>
#include <kxmlguifactory.h>
#include <QtCore/QTimer>
#include <kvbox.h>

typedef KParts::GenericFactory<KMultiPart> KMultiPartFactory; // factory for the part
K_EXPORT_COMPONENT_FACTORY( libkmultipart /*library name*/, KMultiPartFactory )

//#define DEBUG_PARSING

class KLineParser
{
public:
    KLineParser() {
        m_lineComplete = false;
    }
    void addChar( char c, bool storeNewline ) {
        if ( !storeNewline && c == '\r' )
            return;
        Q_ASSERT( !m_lineComplete );
        if ( storeNewline || c != '\n' ) {
            int sz = m_currentLine.size();
            m_currentLine.resize( sz+1 );
            m_currentLine[sz] = c;
        }
        if ( c == '\n' )
            m_lineComplete = true;
    }
    bool isLineComplete() const {
        return m_lineComplete;
    }
    QByteArray currentLine() const {
        return m_currentLine;
    }
    void clearLine() {
        Q_ASSERT( m_lineComplete );
        reset();
    }
    void reset() {
        m_currentLine.resize( 0 );
        m_lineComplete = false;
    }
private:
    QByteArray m_currentLine;
    bool m_lineComplete; // true when ending with '\n'
};

/* testcase:
   Content-type: multipart/mixed;boundary=ThisRandomString

--ThisRandomString
Content-type: text/plain

Data for the first object.

--ThisRandomString
Content-type: text/plain

Data for the second and last object.

--ThisRandomString--
*/


KMultiPart::KMultiPart( QWidget *parentWidget,
                        QObject *parent, const QStringList& )
    : KParts::ReadOnlyPart( parent )
{
    m_filter = 0L;

    setComponentData( KMultiPartFactory::componentData() );

    KVBox *box = new KVBox( parentWidget );
    setWidget( box );

    m_extension = new KParts::BrowserExtension( this );

    m_part = 0L;
    m_isHTMLPart = false;
    m_job = 0L;
    m_lineParser = new KLineParser;
    m_tempFile = 0L;

    m_timer = new QTimer( this );
    connect( m_timer, SIGNAL( timeout() ), this, SLOT( slotProgressInfo() ) );
}

KMultiPart::~KMultiPart()
{
    // important: delete the nested part before the part or qobject destructor runs.
    // we now delete the nested part which deletes the part's widget which makes
    // _OUR_ m_widget 0 which in turn avoids our part destructor to delete the
    // widget ;-)
    // ### additional note: it _can_ be that the part has been deleted before:
    // when we're in a html frameset and the view dies first, then it will also
    // kill the htmlpart
    if ( m_part )
        delete static_cast<KParts::ReadOnlyPart *>( m_part );
    delete m_job;
    delete m_lineParser;
    if ( m_tempFile ) {
        m_tempFile->setAutoRemove( true );
        delete m_tempFile;
    }
    delete m_filter;
    m_filter = 0L;
}


void KMultiPart::startHeader()
{
    m_bParsingHeader = true; // we expect a header to come first
    m_bGotAnyHeader = false;
    m_gzip = false;
    // just to be sure for now
    delete m_filter;
    m_filter = 0L;
}


bool KMultiPart::openUrl( const KUrl &url )
{
    setUrl(url);
    m_lineParser->reset();
    startHeader();

    //m_mimeType = arguments().mimeType();

    // Hmm, args.reload is set to true when reloading, but this doesn't seem to be enough...
    // I get "HOLD: Reusing held slave for <url>", and the old data

    m_job = KIO::get( url,
                      arguments().reload() ? KIO::Reload : KIO::NoReload,
                      KIO::HideProgressInfo );

    emit started( 0 /*m_job*/ ); // don't pass the job, it would interfere with our own infoMessage

    connect( m_job, SIGNAL( result( KJob * ) ),
             this, SLOT( slotJobFinished( KJob * ) ) );
    connect( m_job, SIGNAL( data( KIO::Job *, const QByteArray & ) ),
             this, SLOT( slotData( KIO::Job *, const QByteArray & ) ) );

    m_numberOfFrames = 0;
    m_numberOfFramesSkipped = 0;
    m_totalNumberOfFrames = 0;
    m_qtime.start();
    m_timer->start( 1000 ); //1s

    return true;
}

// Yes, libkdenetwork's has such a parser already (MultiPart),
// but it works on the complete string, expecting the whole data to be available....
// The version here is asynchronous.
void KMultiPart::slotData( KIO::Job *job, const QByteArray &data )
{
    if (m_boundary.isNull())
    {
       QString tmp = job->queryMetaData("media-boundary");
       kDebug() << "Got Boundary from kio-http '" << tmp << "'";
       if ( !tmp.isEmpty() ) {
           if (tmp.startsWith(QLatin1String("--")))
               m_boundary = tmp.toLatin1();
           else
               m_boundary = QByteArray("--")+tmp.toLatin1();
           m_boundaryLength = m_boundary.length();
       }
    }
    // Append to m_currentLine until eol
    for ( int i = 0; i < data.size() ; ++i )
    {
        // Store char. Skip if '\n' and currently parsing a header.
        m_lineParser->addChar( data[i], !m_bParsingHeader );
        if ( m_lineParser->isLineComplete() )
        {
            QByteArray lineData = m_lineParser->currentLine();
#ifdef DEBUG_PARSING
            kDebug() << "lineData.size()=" << lineData.size();
#endif
            QByteArray line( lineData.data(), lineData.size()+1 ); // deep copy
            // 0-terminate the data, but only for the line-based tests below
            // We want to keep the raw data in case it ends up in sendData()
            int sz = line.size();
            if ( sz > 0 )
                line[sz-1] = '\0';
#ifdef DEBUG_PARSING
            kDebug() << "[" << m_bParsingHeader << "] line='" << line << "'";
#endif
            if ( m_bParsingHeader )
            {
                if ( !line.isEmpty() )
                    m_bGotAnyHeader = true;
                if ( m_boundary.isNull() )
                {
                    if ( !line.isEmpty() ) {
#ifdef DEBUG_PARSING
                        kDebug() << "Boundary is " << line;
#endif
                        m_boundary = line;
                        m_boundaryLength = m_boundary.length();
                    }
                }
                else if ( !qstrnicmp( line.data(), "Content-Encoding:", 17 ) )
                {
                    QString encoding = QString::fromLatin1(line.data()+17).trimmed().toLower();
                    if (encoding == "gzip" || encoding == "x-gzip") {
                        m_gzip = true;
                    } else {
                        kDebug() << "FIXME: unhandled encoding type in KMultiPart: " << encoding;
                    }
                }
                // parse Content-Type
                else if ( !qstrnicmp( line.data(), "Content-Type:", 13 ) )
                {
                    Q_ASSERT( m_nextMimeType.isNull() );
                    m_nextMimeType = QString::fromLatin1( line.data() + 14 ).trimmed();
                    int semicolon = m_nextMimeType.indexOf( ';' );
                    if ( semicolon != -1 )
                        m_nextMimeType = m_nextMimeType.left( semicolon );
                    kDebug() << "m_nextMimeType=" << m_nextMimeType;
                }
                // Empty line, end of headers (if we had any header line before)
                else if ( line.isEmpty() && m_bGotAnyHeader )
                {
                    m_bParsingHeader = false;
#ifdef DEBUG_PARSING
                    kDebug() << "end of headers";
#endif
                    startOfData();
                }
                // First header (when we know it from kio_http)
                else if ( line == m_boundary )
                    ; // nothing to do
                else if ( !line.isEmpty() ) // this happens with e.g. Set-Cookie:
                    kDebug() << "Ignoring header " << line;
            } else {
                if ( !qstrncmp( line, m_boundary, m_boundaryLength ) )
                {
#ifdef DEBUG_PARSING
                    kDebug() << "boundary found!";
                    kDebug() << "after it is " << line.data() + m_boundaryLength;
#endif
                    // Was it the very last boundary ?
                    if ( !qstrncmp( line.data() + m_boundaryLength, "--", 2 ) )
                    {
#ifdef DEBUG_PARSING
                        kDebug() << "Completed!";
#endif
                        endOfData();
                        emit completed();
                    } else
                    {
                        char nextChar = *(line.data() + m_boundaryLength);
#ifdef DEBUG_PARSING
                        kDebug() << "KMultiPart::slotData nextChar='" << nextChar << "'";
#endif
                        if ( nextChar == '\n' || nextChar == '\r' ) {
                            endOfData();
                            startHeader();
                        }
                        else {
                            // otherwise, false hit, it has trailing stuff
                            sendData( lineData );
                        }
                    }
                } else {
                    // send to part
                    sendData( lineData );
                }
            }
            m_lineParser->clearLine();
        }
    }
}

void KMultiPart::setPart( const QString& mimeType )
{
    KXMLGUIFactory *guiFactory = factory();
    if ( guiFactory ) // seems to be 0 when restoring from SM
        guiFactory->removeClient( this );
    kDebug() << "KMultiPart::setPart " << mimeType;
    delete m_part;
    // Try to find an appropriate viewer component
    m_part = KParts::ComponentFactory::createPartInstanceFromQuery<KParts::ReadOnlyPart>
             ( m_mimeType, QString(), widget(), this );
    if ( !m_part ) {
        // TODO launch external app
        KMessageBox::error( widget(), i18n("No handler found for %1.", m_mimeType) );
        return;
    }
    // By making the part a child XMLGUIClient of ours, we get its GUI merged in.
    insertChildClient( m_part );
    m_part->widget()->show();

    connect( m_part, SIGNAL( completed() ),
             this, SLOT( slotPartCompleted() ) );

    m_isHTMLPart = ( mimeType == "text/html" );
    KParts::BrowserExtension* childExtension = KParts::BrowserExtension::childObject( m_part );

    if ( childExtension )
    {

        // Forward signals from the part's browser extension
        // this is very related (but not exactly like) KHTMLPart::processObjectRequest

        connect( childExtension, SIGNAL( openURLNotify() ),
                 m_extension, SIGNAL( openURLNotify() ) );

        connect( childExtension, SIGNAL( openUrlRequestDelayed( const KUrl &, const KParts::OpenUrlArguments&, const KParts::BrowserArguments& ) ),
                 m_extension, SIGNAL( openUrlRequest( const KUrl &, const KParts::OpenUrlArguments&, const KParts::BrowserArguments & ) ) );

        connect( childExtension, SIGNAL( createNewWindow( const KUrl &, const KParts::OpenUrlArguments&, const KParts::BrowserArguments&, const KParts::WindowArgs &, KParts::ReadOnlyPart** ) ),
                 m_extension, SIGNAL( createNewWindow( const KUrl &, const KParts::OpenUrlArguments&, const KParts::BrowserArguments& , const KParts::WindowArgs &, KParts::ReadOnlyPart**) ) );

        // Keep in sync with khtml_part.cpp
        connect( childExtension, SIGNAL(popupMenu(QPoint,KFileItemList,KParts::OpenUrlArguments,KParts::BrowserArguments,KParts::BrowserExtension::PopupFlags,KParts::BrowserExtension::ActionGroupMap)),
             m_extension, SIGNAL(popupMenu(QPoint,KFileItemList,KParts::OpenUrlArguments,KParts::BrowserArguments,KParts::BrowserExtension::PopupFlags,KParts::BrowserExtension::ActionGroupMap)) );
        connect( childExtension, SIGNAL(popupMenu(QPoint,KUrl,mode_t,KParts::OpenUrlArguments,KParts::BrowserArguments,KParts::BrowserExtension::PopupFlags,KParts::BrowserExtension::ActionGroupMap)),
             m_extension, SIGNAL(popupMenu(QPoint,KUrl,mode_t,KParts::OpenUrlArguments,KParts::BrowserArguments,KParts::BrowserExtension::PopupFlags,KParts::BrowserExtension::ActionGroupMap)) );

        if ( m_isHTMLPart )
            connect( childExtension, SIGNAL( infoMessage( const QString & ) ),
                     m_extension, SIGNAL( infoMessage( const QString & ) ) );
        // For non-HTML we prefer to show our infoMessage ourselves.

        childExtension->setBrowserInterface( m_extension->browserInterface() );

        connect( childExtension, SIGNAL( enableAction( const char *, bool ) ),
                 m_extension, SIGNAL( enableAction( const char *, bool ) ) );
        connect( childExtension, SIGNAL( setLocationBarURL( const QString& ) ),
                 m_extension, SIGNAL( setLocationBarURL( const QString& ) ) );
        connect( childExtension, SIGNAL( setIconURL( const KUrl& ) ),
                 m_extension, SIGNAL( setIconURL( const KUrl& ) ) );
        connect( childExtension, SIGNAL( loadingProgress( int ) ),
                 m_extension, SIGNAL( loadingProgress( int ) ) );
        if ( m_isHTMLPart ) // for non-HTML we have our own
            connect( childExtension, SIGNAL( speedProgress( int ) ),
                     m_extension, SIGNAL( speedProgress( int ) ) );
        connect( childExtension, SIGNAL( selectionInfo( const KFileItemList& ) ),
                 m_extension, SIGNAL( selectionInfo( const KFileItemList& ) ) );
        connect( childExtension, SIGNAL( selectionInfo( const QString& ) ),
                 m_extension, SIGNAL( selectionInfo( const QString& ) ) );
        connect( childExtension, SIGNAL( selectionInfo( const KUrl::List& ) ),
                 m_extension, SIGNAL( selectionInfo( const KUrl::List& ) ) );
        connect( childExtension, SIGNAL( mouseOverInfo( const KFileItem& ) ),
                 m_extension, SIGNAL( mouseOverInfo( const KFileItem& ) ) );
        connect( childExtension, SIGNAL( moveTopLevelWidget( int, int ) ),
                 m_extension, SIGNAL( moveTopLevelWidget( int, int ) ) );
        connect( childExtension, SIGNAL( resizeTopLevelWidget( int, int ) ),
                 m_extension, SIGNAL( resizeTopLevelWidget( int, int ) ) );
    }

    m_partIsLoading = false;
    // Load the part's plugins too.
    // ###### This is a hack. The bug is that KHTMLPart doesn't load its plugins
    // if className != "Browser/View".
    loadPlugins( this, m_part, m_part->componentData() );
    // Get the part's GUI to appear
    if ( guiFactory )
        guiFactory->addClient( this );
}

void KMultiPart::startOfData()
{
    kDebug() << "KMultiPart::startOfData";
    Q_ASSERT( !m_nextMimeType.isNull() );
    if( m_nextMimeType.isNull() )
        return;

    if ( m_gzip )
    {
        m_filter = new HTTPFilterGZip;
        connect( m_filter, SIGNAL( output( const QByteArray& ) ), this, SLOT( reallySendData( const QByteArray& ) ) );
    }

    if ( m_mimeType != m_nextMimeType )
    {
        // Need to switch parts (or create the initial one)
        m_mimeType = m_nextMimeType;
        setPart( m_mimeType );
    }
    Q_ASSERT( m_part );
    // Pass args (e.g. reload)
    m_part->setArguments( arguments() );
    KParts::BrowserExtension* childExtension = KParts::BrowserExtension::childObject( m_part );
    if ( childExtension )
        childExtension->setBrowserArguments( m_extension->browserArguments() );

    m_nextMimeType.clear();
    if ( m_tempFile ) {
        m_tempFile->setAutoRemove( true );
        delete m_tempFile;
        m_tempFile = 0;
    }
    if ( m_isHTMLPart )
    {
        KHTMLPart* htmlPart = static_cast<KHTMLPart *>( static_cast<KParts::ReadOnlyPart *>( m_part ) );
        htmlPart->begin( url() );
    }
    else
    {
        // ###### TODO use a QByteArray and a data: URL instead
        m_tempFile = new KTemporaryFile;
        m_tempFile->open();
    }
}

void KMultiPart::sendData( const QByteArray& line )
{
    if ( m_filter )
    {
        m_filter->slotInput( line );
    }
    else
    {
        reallySendData( line );
    }
}

void KMultiPart::reallySendData( const QByteArray& line )
{
    if ( m_isHTMLPart )
    {
        KHTMLPart* htmlPart = static_cast<KHTMLPart *>( static_cast<KParts::ReadOnlyPart *>( m_part ) );
        htmlPart->write( line.data(), line.size() );
    }
    else if ( m_tempFile )
    {
        m_tempFile->write( line.data(), line.size() );
    }
}

void KMultiPart::endOfData()
{
    Q_ASSERT( m_part );
    if ( m_isHTMLPart )
    {
        KHTMLPart* htmlPart = static_cast<KHTMLPart *>( static_cast<KParts::ReadOnlyPart *>( m_part ) );
        htmlPart->end();
    } else if ( m_tempFile )
    {
        m_tempFile->close();
        if ( m_partIsLoading )
        {
            // The part is still loading the last data! Let it proceed then
            // Otherwise we'd keep canceling it, and nothing would ever show up...
            kDebug() << "KMultiPart::endOfData part isn't ready, skipping frame";
            ++m_numberOfFramesSkipped;
            m_tempFile->setAutoRemove( true );
        }
        else
        {
            kDebug() << "KMultiPart::endOfData opening " << m_tempFile->fileName();
            KUrl url;
            url.setPath( m_tempFile->fileName() );
            m_partIsLoading = true;
            (void) m_part->openUrl( url );
        }
        delete m_tempFile;
        m_tempFile = 0L;
    }
}

void KMultiPart::slotPartCompleted()
{
    if ( !m_isHTMLPart )
    {
        Q_ASSERT( m_part );
        // Delete temp file used by the part
        Q_ASSERT( m_part->url().isLocalFile() );
	kDebug() << "slotPartCompleted deleting " << m_part->url().toLocalFile();
        (void) unlink( QFile::encodeName( m_part->url().toLocalFile() ) );
        m_partIsLoading = false;
        ++m_numberOfFrames;
        // Do not emit completed from here.
    }
}

bool KMultiPart::closeUrl()
{
    m_timer->stop();
    if ( m_part )
        return m_part->closeUrl();
    return true;
}

void KMultiPart::guiActivateEvent( KParts::GUIActivateEvent * )
{
    // Not public!
    //if ( m_part )
    //    m_part->guiActivateEvent( e );
}

void KMultiPart::slotJobFinished( KJob *job )
{
    if ( job->error() )
    {
        // TODO use khtml's error:// scheme
        job->uiDelegate()->showErrorMessage();
        emit canceled( job->errorString() );
    }
    else
    {
        /*if ( m_khtml->view()->contentsY() == 0 )
        {
            const KParts::OpenUrlArguments args = arguments();
            m_khtml->view()->setContentsPos( args.xOffset(), args.yOffset() );
        }*/

        emit completed();

        //QTimer::singleShot( 0, this, SLOT( updateWindowCaption() ) );
    }
    m_job = 0L;
}

void KMultiPart::slotProgressInfo()
{
    int time = m_qtime.elapsed();
    if ( !time ) return;
    if ( m_totalNumberOfFrames == m_numberOfFrames + m_numberOfFramesSkipped )
        return; // No change, don't overwrite statusbar messages if any
    //kDebug() << m_numberOfFrames << " in " << time << " milliseconds";
    QString str( "%1 frames per second, %2 frames skipped per second" );
    str = str.arg( 1000.0 * (double)m_numberOfFrames / (double)time );
    str = str.arg( 1000.0 * (double)m_numberOfFramesSkipped / (double)time );
    m_totalNumberOfFrames = m_numberOfFrames + m_numberOfFramesSkipped;
    //kDebug() << str;
    emit m_extension->infoMessage( str );
}

KAboutData* KMultiPart::createAboutData()
{
    KAboutData* aboutData = new KAboutData( "kmultipart", 0, ki18n("KMultiPart"),
                                            "0.1",
                                            ki18n( "Embeddable component for multipart/mixed" ),
                                            KAboutData::License_GPL,
                                            ki18n("Copyright 2001, David Faure <email>david@mandrakesoft.com</email>"));
    return aboutData;
}

#if 0
KMultiPartBrowserExtension::KMultiPartBrowserExtension( KMultiPart *parent, const char *name )
    : KParts::BrowserExtension( parent, name )
{
    m_imgPart = parent;
}

int KMultiPartBrowserExtension::xOffset()
{
    return m_imgPart->doc()->view()->contentsX();
}

int KMultiPartBrowserExtension::yOffset()
{
    return m_imgPart->doc()->view()->contentsY();
}

void KMultiPartBrowserExtension::print()
{
    static_cast<KHTMLPartBrowserExtension *>( m_imgPart->doc()->browserExtension() )->print();
}

void KMultiPartBrowserExtension::reparseConfiguration()
{
    static_cast<KHTMLPartBrowserExtension *>( m_imgPart->doc()->browserExtension() )->reparseConfiguration();
    m_imgPart->doc()->setAutoloadImages( true );
}
#endif

#include "kmultipart.moc"
