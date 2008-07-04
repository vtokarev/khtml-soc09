/* This file is part of the KDE libraries
    Copyright 2000       Stephan Kulow <coolo@kde.org>
    Copyright 2000-2006  David Faure <faure@kde.org>
    Copyright 2000       Waldo Bastian <bastian@kde.org>

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

#include "deletejob.h"

#include "kmimetype.h"
#include "scheduler.h"
#include "kdirwatch.h"
#include "kprotocolmanager.h"
#include "jobuidelegate.h"
#include <kdirnotify.h>
#include <kuiserverjobtracker.h>

#include <kauthorized.h>
#include <klocale.h>
#include <kdebug.h>
#include <kde_file.h>

#include <assert.h>
#include <stdlib.h>
#include <time.h>

#include <QtCore/QTimer>
#include <QtCore/QFile>
#include <QPointer>

#include "job_p.h"

namespace KIO
{
    enum DeleteJobState {
        DELETEJOB_STATE_STATING,
        DELETEJOB_STATE_LISTING,
        DELETEJOB_STATE_DELETING_FILES,
        DELETEJOB_STATE_DELETING_DIRS
    };

    class DeleteJobPrivate: public KIO::JobPrivate
    {
    public:
        DeleteJobPrivate(const KUrl::List& src)
            : state( DELETEJOB_STATE_STATING )
            , m_totalSize( 0 )
            , m_processedSize( 0 )
            , m_fileProcessedSize( 0 )
            , m_processedFiles( 0 )
            , m_processedDirs( 0 )
            , m_totalFilesDirs( 0 )
            , m_srcList( src )
            , m_currentStat( m_srcList.begin() )
            , m_reportTimer( 0 )
        {
        }
        DeleteJobState state;
        KIO::filesize_t m_totalSize;
        KIO::filesize_t m_processedSize;
        KIO::filesize_t m_fileProcessedSize;
        int m_processedFiles;
        int m_processedDirs;
        int m_totalFilesDirs;
        KUrl m_currentURL;
        KUrl::List files;
        KUrl::List symlinks;
        KUrl::List dirs;
        KUrl::List m_srcList;
        KUrl::List::Iterator m_currentStat;
	QStringList m_parentDirs;
        QTimer *m_reportTimer;

        void statNextSrc();
        void deleteNextFile();
        void deleteNextDir();
        /**
         * Forward signal from subjob
         */
        void slotProcessedSize( KJob*, qulonglong data_size );
        void slotReport();
        void slotStart();
        void slotEntries( KIO::Job*, const KIO::UDSEntryList& list );

        Q_DECLARE_PUBLIC(DeleteJob)

        static inline DeleteJob *newJob(const KUrl::List &src, JobFlags flags)
        {
            DeleteJob *job = new DeleteJob(*new DeleteJobPrivate(src));
            job->setUiDelegate(new JobUiDelegate);
            if (!(flags & HideProgressInfo))
                KIO::getJobTracker()->registerJob(job);
            return job;
        }
    };

} // namespace KIO

using namespace KIO;

DeleteJob::DeleteJob(DeleteJobPrivate &dd)
    : Job(dd)
{
    d_func()->m_reportTimer = new QTimer(this);
    connect(d_func()->m_reportTimer,SIGNAL(timeout()),this,SLOT(slotReport()));
    //this will update the report dialog with 5 Hz, I think this is fast enough, aleXXX
    d_func()->m_reportTimer->start( 200 );

    QTimer::singleShot(0, this, SLOT(slotStart()));
}

DeleteJob::~DeleteJob()
{
}

KUrl::List DeleteJob::urls() const
{
    return d_func()->m_srcList;
}

void DeleteJobPrivate::slotStart()
{
    statNextSrc();
}

void DeleteJobPrivate::slotReport()
{
   Q_Q(DeleteJob);
   emit q->deleting( q, m_currentURL );
   JobPrivate::emitDeleting( q, m_currentURL);

   switch( state ) {
        case DELETEJOB_STATE_STATING:
        case DELETEJOB_STATE_LISTING:
            q->setTotalAmount(KJob::Bytes, m_totalSize);
            q->setTotalAmount(KJob::Files, files.count());
            q->setTotalAmount(KJob::Directories, dirs.count());
            break;
        case DELETEJOB_STATE_DELETING_DIRS:
            q->setProcessedAmount(KJob::Directories, m_processedDirs);
            q->emitPercent( m_processedFiles + m_processedDirs, m_totalFilesDirs );
            break;
        case DELETEJOB_STATE_DELETING_FILES:
            q->setProcessedAmount(KJob::Files, m_processedFiles);
            q->emitPercent( m_processedFiles, m_totalFilesDirs );
            break;
   }
}


void DeleteJobPrivate::slotEntries(KIO::Job* job, const UDSEntryList& list)
{
    UDSEntryList::ConstIterator it = list.begin();
    const UDSEntryList::ConstIterator end = list.end();
    for (; it != end; ++it)
    {
        const UDSEntry& entry = *it;
        const QString displayName = entry.stringValue( KIO::UDSEntry::UDS_NAME );

        assert(!displayName.isEmpty());
        if (displayName != ".." && displayName != ".")
        {
            KUrl url;
            const QString urlStr = entry.stringValue( KIO::UDSEntry::UDS_URL );
            if ( !urlStr.isEmpty() )
                url = urlStr;
            else {
                url = ((SimpleJob *)job)->url(); // assumed to be a dir
                url.addPath( displayName );
            }

            m_totalSize += (KIO::filesize_t)entry.numberValue( KIO::UDSEntry::UDS_SIZE, 0 );

            //kDebug(7007) << displayName << "(" << url << ")";
            if ( entry.isLink() )
                symlinks.append( url );
            else if ( entry.isDir() )
                dirs.append( url );
            else
                files.append( url );
        }
    }
}


void DeleteJobPrivate::statNextSrc()
{
    Q_Q(DeleteJob);
    //kDebug(7007) << "statNextSrc";
    if ( m_currentStat != m_srcList.end() )
    {
        m_currentURL = (*m_currentStat);

        // if the file system doesn't support deleting, we do not even stat
        if (!KProtocolManager::supportsDeleting(m_currentURL)) {
            QPointer<DeleteJob> that = q;
            ++m_currentStat;
            emit q->warning( q, buildErrorString(ERR_CANNOT_DELETE, m_currentURL.prettyUrl()) );
            if (that)
                statNextSrc();
            return;
        }
        // Stat it
        state = DELETEJOB_STATE_STATING;
        KIO::SimpleJob * job = KIO::stat( m_currentURL, StatJob::SourceSide, 1, KIO::HideProgressInfo );
        Scheduler::scheduleJob(job);
        //kDebug(7007) << "KIO::stat (DeleteJob) " << m_currentURL;
        q->addSubjob(job);
    } else
    {
        m_totalFilesDirs = files.count()+symlinks.count() + dirs.count();
        slotReport();
        // Now we know which dirs hold the files we're going to delete.
        // To speed things up and prevent double-notification, we disable KDirWatch
        // on those dirs temporarily (using KDirWatch::self, that's the instanced
        // used by e.g. kdirlister).
        for ( QStringList::Iterator it = m_parentDirs.begin() ; it != m_parentDirs.end() ; ++it )
            KDirWatch::self()->stopDirScan( *it );
        state = DELETEJOB_STATE_DELETING_FILES;
        deleteNextFile();
    }
}

void DeleteJobPrivate::deleteNextFile()
{
    Q_Q(DeleteJob);
    //kDebug(7007) << "deleteNextFile";
    if ( !files.isEmpty() || !symlinks.isEmpty() )
    {
        SimpleJob *job;
        do {
            // Take first file to delete out of list
            KUrl::List::Iterator it = files.begin();
            bool isLink = false;
            if ( it == files.end() ) // No more files
            {
                it = symlinks.begin(); // Pick up a symlink to delete
                isLink = true;
            }
            // Normal deletion
            // If local file, try do it directly
            if ( (*it).isLocalFile() && unlink( QFile::encodeName((*it).path()) ) == 0 ) {
                //kdDebug(7007) << "DeleteJob deleted" << (*it).path();
                job = 0;
                m_processedFiles++;
                if ( m_processedFiles % 300 == 0 || m_totalFilesDirs < 300) { // update progress info every 300 files
                    m_currentURL = *it;
                    slotReport();
                }
            } else
            { // if remote - or if unlink() failed (we'll use the job's error handling in that case)
                job = KIO::file_delete( *it, KIO::HideProgressInfo );
                Scheduler::scheduleJob(job);
                m_currentURL=(*it);
            }
            if ( isLink )
                symlinks.erase(it);
            else
                files.erase(it);
            if ( job ) {
                q->addSubjob(job);
                return;
            }
            // loop only if direct deletion worked (job=0) and there is something else to delete
        } while (!job && (!files.isEmpty() || !symlinks.isEmpty()));
    }
    state = DELETEJOB_STATE_DELETING_DIRS;
    deleteNextDir();
}

void DeleteJobPrivate::deleteNextDir()
{
    Q_Q(DeleteJob);
    if ( !dirs.isEmpty() ) // some dirs to delete ?
    {
        do {
            // Take first dir to delete out of list - last ones first !
            KUrl::List::Iterator it = --dirs.end();
            // If local dir, try to rmdir it directly
            if ( (*it).isLocalFile() && ::rmdir( QFile::encodeName((*it).path()) ) == 0 ) {

                m_processedDirs++;
                if ( m_processedDirs % 100 == 0 ) { // update progress info every 100 dirs
                    m_currentURL = *it;
                    slotReport();
                }
            } else {
                SimpleJob* job;
                if ( KProtocolManager::canDeleteRecursive( *it ) ) {
                    // If the ioslave supports recursive deletion of a directory, then
                    // we only need to send a single CMD_DEL command, so we use file_delete :)
                    job = KIO::file_delete( *it, KIO::HideProgressInfo );
                } else {
                    job = KIO::rmdir( *it );
                }
                Scheduler::scheduleJob(job);
                dirs.erase(it);
                q->addSubjob( job );
                return;
            }
            dirs.erase(it);
        } while ( !dirs.isEmpty() );
    }

    // Re-enable watching on the dirs that held the deleted files
    for ( QStringList::Iterator it = m_parentDirs.begin() ; it != m_parentDirs.end() ; ++it )
        KDirWatch::self()->restartDirScan( *it );

    // Finished - tell the world
    if ( !m_srcList.isEmpty() )
    {
        //kDebug(7007) << "KDirNotify'ing FilesRemoved " << m_srcList.toStringList();
        org::kde::KDirNotify::emitFilesRemoved( m_srcList.toStringList() );
    }
    if (m_reportTimer!=0)
       m_reportTimer->stop();
    q->emitResult();
}

// Note: I don't think this slot is connected to anywhere! -thiago
void DeleteJobPrivate::slotProcessedSize( KJob*, qulonglong data_size )
{
   Q_Q(DeleteJob);
   // Note: this is the same implementation as CopyJob::slotProcessedSize but
   // it's different from FileCopyJob::slotProcessedSize - which is why this
   // is not in Job.

   m_fileProcessedSize = data_size;
   q->setProcessedAmount(KJob::Bytes, m_processedSize + m_fileProcessedSize);

   //kDebug(7007) << (unsigned int) (m_processedSize + m_fileProcessedSize);

   q->setProcessedAmount(KJob::Bytes, m_processedSize + m_fileProcessedSize);

   // calculate percents
   if ( m_totalSize == 0 )
      q->setPercent( 100 );
   else
      q->setPercent( (unsigned long)(( (float)(m_processedSize + m_fileProcessedSize) / (float)m_totalSize ) * 100.0) );
}

void DeleteJob::slotResult( KJob *job )
{
    Q_D(DeleteJob);
    switch ( d->state )
    {
    case DELETEJOB_STATE_STATING:
    {
        // Was there an error while stating ?
        if (job->error() )
        {
            // Probably : doesn't exist
            Job::slotResult( job ); // will set the error and emit result(this)
            return;
        }

        const UDSEntry entry = static_cast<StatJob*>(job)->statResult();
        const KUrl url = static_cast<SimpleJob*>(job)->url();
        const bool isLink = entry.isLink();

        removeSubjob( job );
        assert( !hasSubjobs() );

        // Is it a file or a dir ?
        if (entry.isDir() && !isLink)
        {
            // Add toplevel dir in list of dirs
            d->dirs.append( url );
            if ( url.isLocalFile() && !d->m_parentDirs.contains( url.path(KUrl::RemoveTrailingSlash) ) )
              d->m_parentDirs.append( url.path(KUrl::RemoveTrailingSlash) );

            if ( !KProtocolManager::canDeleteRecursive( url ) ) {
                //kDebug(7007) << " Target is a directory ";
                // List it
                d->state = DELETEJOB_STATE_LISTING;
                ListJob *newjob = KIO::listRecursive( url, KIO::HideProgressInfo );
                newjob->setUnrestricted(true); // No KIOSK restrictions
                Scheduler::scheduleJob(newjob);
                connect(newjob, SIGNAL(entries( KIO::Job *,
                                                const KIO::UDSEntryList& )),
                        SLOT( slotEntries( KIO::Job*,
                                           const KIO::UDSEntryList& )));
                addSubjob(newjob);
            } else {
                ++d->m_currentStat;
                d->statNextSrc();
            }
        }
        else
        {
            if ( isLink ) {
                //kDebug(7007) << " Target is a symlink";
                d->symlinks.append( url );
            } else {
                //kDebug(7007) << " Target is a file";
                d->files.append( url );
            }
            if ( url.isLocalFile() && !d->m_parentDirs.contains( url.directory(KUrl::ObeyTrailingSlash) ) )
                d->m_parentDirs.append( url.directory(KUrl::ObeyTrailingSlash) );
            ++d->m_currentStat;
            d->statNextSrc();
        }
    }
        break;
    case DELETEJOB_STATE_LISTING:
        if ( job->error() )
        {
            // Try deleting nonetheless, it may be empty (and non-listable)
        }
        removeSubjob( job );
        assert( !hasSubjobs() );
        ++d->m_currentStat;
        d->statNextSrc();
        break;
    case DELETEJOB_STATE_DELETING_FILES:
        if ( job->error() )
        {
            Job::slotResult( job ); // will set the error and emit result(this)
            return;
        }
        removeSubjob( job );
        assert( !hasSubjobs() );
        d->m_processedFiles++;

        d->deleteNextFile();
        break;
    case DELETEJOB_STATE_DELETING_DIRS:
        if ( job->error() )
        {
            Job::slotResult( job ); // will set the error and emit result(this)
            return;
        }
        removeSubjob( job );
        assert( !hasSubjobs() );
        d->m_processedDirs++;
        //emit processedAmount( this, KJob::Directories, d->m_processedDirs );
        //emitPercent( d->m_processedFiles + d->m_processedDirs, d->m_totalFilesDirs );

        d->deleteNextDir();
        break;
    default:
        assert(0);
    }
}

DeleteJob *KIO::del( const KUrl& src, JobFlags flags )
{
    KUrl::List srcList;
    srcList.append( src );
    return DeleteJobPrivate::newJob(srcList, flags);
}

DeleteJob *KIO::del( const KUrl::List& src, JobFlags flags )
{
    return DeleteJobPrivate::newJob(src, flags);
}

#include "deletejob.moc"
