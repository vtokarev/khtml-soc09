/*
   Copyright (C) 2000 David Faure <faure@kde.org>

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

#ifndef SLAVEBASE_H
#define SLAVEBASE_H

#include <kio/global.h>
#include <kio/udsentry.h>
#include <kio/authinfo.h>
#include <kio/jobclasses.h> // for KIO::JobFlags
#include <klocale.h>

#include <QtCore/QByteArray>

class KConfigGroup;
class KRemoteEncoding;
class KUrl;

namespace KIO {

class Connection;
class SlaveBasePrivate;

/**
 * There are two classes that specifies the protocol between application (job)
 * and kioslave. SlaveInterface is the class to use on the application end,
 * SlaveBase is the one to use on the slave end.
 *
 * Slave implementations should simply inherit SlaveBase
 *
 * A call to foo() results in a call to slotFoo() on the other end.
 */
class KIO_EXPORT SlaveBase
{
public:
    SlaveBase( const QByteArray &protocol, const QByteArray &pool_socket, const QByteArray &app_socket);
    virtual ~SlaveBase();

    /**
     * @internal
     * Terminate the slave by calling the destructor and then ::exit()
     */
    void exit();

    /**
     * @internal
     */
    void dispatchLoop();

    ///////////
    // Message Signals to send to the job
    ///////////

    /**
     * Sends data in the slave to the job (i.e. in get).
     *
     * To signal end of data, simply send an empty
     * QByteArray().
     *
     * @param data the data read by the slave
     */
    void data( const QByteArray &data );

    /**
     * Asks for data from the job.
     * @see readData
     */
    void dataReq( );

    /**
     * open succedes
     * @see open
     */
    void opened();

    /**
     * Call to signal an error.
     * This also finishes the job, so you must not call
     * finished() after calling this.
     *
     * If the Error code is KIO::ERR_SLAVE_DEFINED then the
     * _text should contain the complete translated text of
     * of the error message.  This message will be displayed
     * in an KTextBrowser which allows rich text complete
     * with hyper links.  Email links will call the default
     * mailer, "exec:/command arg1 arg2" will be forked and
     * all other links will call the default browser.
     *
     * @see KIO::Error
     * @see KTextBrowser
     * @param _errid the error code from KIO::Error
     * @param _text the rich text error message
     */
    void error( int _errid, const QString &_text );

    /**
     * Call in openConnection, if you reimplement it, when you're done.
     */
    void connected();

    /**
     * Call to signal successful completion of any command
     * besides openConnection and closeConnection. Do not
     * call this after calling error().
     */
    void finished();

    /**
     * Call to signal that data from the sub-URL is needed
     */
    void needSubUrlData();

    /**
     * Used to report the status of the slave.
     * @param host the slave is currently connected to. (Should be
     *        empty if not connected)
     * @param connected Whether an actual network connection exists.
     **/
    void slaveStatus(const QString &host, bool connected);

    /**
     * Call this from stat() to express details about an object, the
     * UDSEntry customarily contains the atoms describing file name, size,
     * mimetype, etc.
     * @param _entry The UDSEntry containing all of the object attributes.
     */
    void statEntry( const UDSEntry& _entry );

    /**
     * Call this in listDir, each time you have a bunch of entries
     * to report.
     * @param _entry The UDSEntry containing all of the object attributes.
     */
    void listEntries( const UDSEntryList& _entry );

    /**
     * Call this at the beginning of put(), to give the size of the existing
     * partial file, if there is one. The @p offset argument notifies the
     * other job (the one that gets the data) about the offset to use.
     * In this case, the boolean returns whether we can indeed resume or not
     * (we can't if the protocol doing the get() doesn't support setting an offset)
     */
    bool canResume( KIO::filesize_t offset );

    /**
     * Call this at the beginning of get(), if the "resume" metadata was set
     * and resuming is implemented by this protocol.
     */
    void canResume();

    ///////////
    // Info Signals to send to the job
    ///////////

    /**
     * Call this in get and copy, to give the total size
     * of the file
     * Call in listDir too, when you know the total number of items.
     */
    void totalSize( KIO::filesize_t _bytes );
    /**
     * Call this during get and copy, once in a while,
     * to give some info about the current state.
     * Don't emit it in listDir, listEntries speaks for itself.
     */
    void processedSize( KIO::filesize_t _bytes );

    void position( KIO::filesize_t _pos );

    void written( KIO::filesize_t _bytes );

    /**
     * Only use this if you can't know in advance the size of the
     * copied data. For example, if you're doing variable bitrate
     * compression of the source.
     *
     * STUB ! Currently unimplemented. Here now for binary compatibility.
     *
     * Call this during get and copy, once in a while,
     * to give some info about the current state.
     * Don't emit it in listDir, listEntries speaks for itself.
     */
    void processedPercent( float percent );

    /**
     * Call this in get and copy, to give the current transfer
     * speed, but only if it can't be calculated out of the size you
     * passed to processedSize (in most cases you don't want to call it)
     */
    void speed( unsigned long _bytes_per_second );

    /**
     * Call this to signal a redirection
     * The job will take care of going to that url.
     */
    void redirection( const KUrl &_url );

    /**
     * Tell that we will only get an error page here.
     * This means: the data you'll get isn't the data you requested,
     * but an error page (usually HTML) that describes an error.
     */
    void errorPage();

    /**
     * Call this in mimetype() and in get(), when you know the mimetype.
     * See mimetype about other ways to implement it.
     */
    void mimeType( const QString &_type );

    /**
     * Call to signal a warning, to be displayed in a dialog box.
     */
    void warning( const QString &msg );

    /**
     * Call to signal a message, to be displayed if the application wants to,
     * for instance in a status bar. Usual examples are "connecting to host xyz", etc.
     */
    void infoMessage( const QString &msg );

    enum MessageBoxType { QuestionYesNo = 1, WarningYesNo = 2, WarningContinueCancel = 3, WarningYesNoCancel = 4, Information = 5, SSLMessageBox = 6 };

    /**
     * Call this to show a message box from the slave
     * @param type type of message box: QuestionYesNo, WarningYesNo, WarningContinueCancel...
     * @param text Message string. May contain newlines.
     * @param caption Message box title.
     * @param buttonYes The text for the first button.
     *                  The default is i18n("&Yes").
     * @param buttonNo  The text for the second button.
     *                  The default is i18n("&No").
     * Note: for ContinueCancel, buttonYes is the continue button and buttonNo is unused.
     *       and for Information, none is used.
     * @return a button code, as defined in KMessageBox, or 0 on communication error.
     */
    int messageBox( MessageBoxType type, const QString &text,
                    const QString &caption = QString(),
                    const QString &buttonYes = i18n("&Yes"),
                    const QString &buttonNo = i18n("&No"));

    /**
     * Call this to show a message box from the slave
     * @param text Message string. May contain newlines.
     * @param type type of message box: QuestionYesNo, WarningYesNo, WarningContinueCancel...
     * @param caption Message box title.
     * @param buttonYes The text for the first button.
     *                  The default is i18n("&Yes").
     * @param buttonNo  The text for the second button.
     *                  The default is i18n("&No").
     * Note: for ContinueCancel, buttonYes is the continue button and buttonNo is unused.
     *       and for Information, none is used.
     * @param dontAskAgain A checkbox is added with which further confirmation can be turned off.
     *        If the checkbox was ticked @p*dontAskAgain will be set to true, otherwise false.
     * @return a button code, as defined in KMessageBox, or 0 on communication error.
     */
    int messageBox( const QString &text, MessageBoxType type,
                    const QString &caption = QString(),
                    const QString &buttonYes = i18n("&Yes"),
                    const QString &buttonNo = i18n("&No"),
                    const QString &dontAskAgainName = QString() );

    /**
     * Sets meta-data to be send to the application before the first
     * data() or finished() signal.
     */
    void setMetaData(const QString &key, const QString &value);

    /**
     * Queries for the existence of a certain config/meta-data entry
     * send by the application to the slave.
     */
    bool hasMetaData(const QString &key) const;

    /**
     * Queries for config/meta-data send by the application to the slave.
     */
    QString metaData(const QString &key) const;


    /**
     * @internal for ForwardingSlaveBase
     * Contains all metadata (but no config) sent by the application to the slave.
     */
    MetaData allMetaData() const;

    /**
     * Returns a configuration object to query config/meta-data information
     * from.
     *
     * The application provides the slave with all configuration information
     * relevant for the current protocol and host.
     */
    KConfigGroup* config();

    /**
     * Returns an object that can translate remote filenames into proper
     * Unicode forms. This encoding can be set by the user.
     */
    KRemoteEncoding* remoteEncoding();


    ///////////
    // Commands sent by the job, the slave has to
    // override what it wants to implement
    ///////////

    /**
     * Set the host
     * @param host
     * @param port
     * @param user
     * @param pass
     * Called directly by createSlave, this is why there is no equivalent in
     * SlaveInterface, unlike the other methods.
     *
     * This method is called whenever a change in host, port or user occurs.
     */
    virtual void setHost(const QString& host, quint16 port, const QString& user, const QString& pass);

    /**
     * Prepare slave for streaming operation
     */
    virtual void setSubUrl(const KUrl&url);

    /**
     * Opens the connection (forced)
     * When this function gets called the slave is operating in
     * connection-oriented mode.
     * When a connection gets lost while the slave operates in
     * connection oriented mode, the slave should report
     * ERR_CONNECTION_BROKEN instead of reconnecting. The user is
     * expected to disconnect the slave in the error handler.
     */
    virtual void openConnection();

    /**
     * Closes the connection (forced)
     * Called when the application disconnects the slave to close
     * any open network connections.
     *
     * When the slave was operating in connection-oriented mode,
     * it should reset itself to connectionless (default) mode.
     */
    virtual void closeConnection();

    /**
     * get, aka read.
     * @param url the full url for this request. Host, port and user of the URL
     *        can be assumed to be the same as in the last setHost() call.
     * The slave emits the data through data
     */
    virtual void get( const KUrl& url );

    /**
     * open.
     * @param url the full url for this request. Host, port and user of the URL
     *        can be assumed to be the same as in the last setHost() call.
     * @param mode see \ref QIODevice::OpenMode
     */
    virtual void open( const KUrl &url, QIODevice::OpenMode mode );

    virtual void read( KIO::filesize_t size );
    virtual void write( const QByteArray &data );
    virtual void seek( KIO::filesize_t offset );
    virtual void close();

    /**
     * put, i.e. write data into a file.
     *
     * @param url where to write the file
     * @param permissions may be -1. In this case no special permission mode is set.
     * @param flags: We support Overwrite here. Hopefully, we're going to
     * support Resume in the future, too.
     * If the file indeed already exists, the slave should NOT apply the
     * permissions change to it.
     * The support for resuming using .part files is done by calling canResume().
     *
     * IMPORTANT: Use the "modified" metadata in order to set the modification time of the file.
     *
     * @see canResume()
     */
    virtual void put( const KUrl& url, int permissions, JobFlags flags );

    /**
     * Finds all details for one file or directory.
     * The information returned is the same as what listDir returns,
     * but only for one file or directory.
     */
    virtual void stat( const KUrl& url );

    /**
     * Finds mimetype for one file or directory.
     *
     * This method should either emit 'mimeType' or it
     * should send a block of data big enough to be able
     * to determine the mimetype.
     *
     * If the slave doesn't reimplement it, a get will
     * be issued, i.e. the whole file will be downloaded before
     * determining the mimetype on it - this is obviously not a
     * good thing in most cases.
     */
    virtual void mimetype( const KUrl& url );

    /**
     * Lists the contents of @p url.
     * The slave should emit ERR_CANNOT_ENTER_DIRECTORY if it doesn't exist,
     * if we don't have enough permissions, or if it is a file
     * It should also emit totalFiles as soon as it knows how many
     * files it will list.
     */
    virtual void listDir( const KUrl& url );

    /**
     * Create a directory
     * @param url path to the directory to create
     * @param permissions the permissions to set after creating the directory
     * (-1 if no permissions to be set)
     * The slave emits ERR_COULD_NOT_MKDIR if failure.
     */
    virtual void mkdir( const KUrl&url, int permissions );

    /**
     * Rename @p oldname into @p newname.
     * If the slave returns an error ERR_UNSUPPORTED_ACTION, the job will
     * ask for copy + del instead.
     *
     * Important: the slave must implement the logic "if the destination already
     * exists, error ERR_DIR_ALREADY_EXIST or ERR_FILE_ALREADY_EXIST".
     * For performance reasons no stat is done in the destination before hand,
     * the slave must do it.
     *
     * @param src where to move the file from
     * @param dest where to move the file to
     * @param flags: We support Overwrite here
     */
    virtual void rename( const KUrl& src, const KUrl& dest, JobFlags flags );

    /**
     * Creates a symbolic link named @p dest, pointing to @p target, which
     * may be a relative or an absolute path.
     * @param target The string that will become the "target" of the link (can be relative)
     * @param dest The symlink to create.
     * @param flags: We support Overwrite here
     */
    virtual void symlink( const QString& target, const KUrl& dest, JobFlags flags );

    /**
     * Change permissions on @p url
     * The slave emits ERR_DOES_NOT_EXIST or ERR_CANNOT_CHMOD
     */
    virtual void chmod( const KUrl& url, int permissions );

    /**
     * Change ownership of @p url
     * The slave emits ERR_DOES_NOT_EXIST or ERR_CANNOT_CHOWN
     */
    virtual void chown( const KUrl& url, const QString& owner, const QString& group );

    /**
     * Sets the modification time for @url
     * For instance this is what CopyJob uses to set mtime on dirs at the end of a copy.
     * It could also be used to set the mtime on any file, in theory.
     * The usual implementation on unix is to call utime(path, &myutimbuf).
     * The slave emits ERR_DOES_NOT_EXIST or ERR_CANNOT_SETTIME
     */
    virtual void setModificationTime( const KUrl& url, const QDateTime& mtime );

    /**
     * Copy @p src into @p dest.
     * If the slave returns an error ERR_UNSUPPORTED_ACTION, the job will
     * ask for get + put instead.
     * @param src where to copy the file from (decoded)
     * @param dest where to copy the file to (decoded)
     * @param permissions may be -1. In this case no special permission mode is set.
     * @param flags: We support Overwrite here
     *
     * Don't forget to set the modification time of @p dest to be the modification time of @p src.
     */
    virtual void copy( const KUrl &src, const KUrl &dest, int permissions, JobFlags flags );

    /**
     * Delete a file or directory.
     * @param url file/directory to delete
     * @param isfile if true, a file should be deleted.
     *               if false, a directory should be deleted.
     */
    virtual void del( const KUrl &url, bool isfile);

    // TODO KDE4: use setLinkDest for kio_file but also kio_remote (#97129)
    // For kio_file it's the same as del+symlink, but for kio_remote it allows
    // to keep the icon etc.

    /**
     * Change the destination of a symlink
     * @param url the url of the symlink to modify
     * @param target the new destination (target) of the symlink
     */
    virtual void setLinkDest( const KUrl& url, const QString& target );

    /**
     * Used for any command that is specific to this slave (protocol)
     * Examples are : HTTP POST, mount and unmount (kio_file)
     *
     * @param data packed data; the meaning is completely dependent on the
     *        slave, but usually starts with an int for the command number.
     * Document your slave's commands, at least in its header file.
     */
    virtual void special( const QByteArray & data );

    /**
     * Used for multiple get. Currently only used foir HTTP pielining
     * support.
     *
     * @param data packed data; Contains number of URLs to fetch, and for
     * each URL the URL itself and its associated MetaData.
     */
    virtual void multiGet( const QByteArray & data );

    /**
     * Called to get the status of the slave. Slave should respond
     * by calling slaveStatus(...)
     */
    virtual void slave_status();

    /**
     * Called by the scheduler to tell the slave that the configuration
     * changed (i.e. proxy settings) .
     */
    virtual void reparseConfiguration();


    /**
     * @return timeout value for connecting to remote host.
     */
    int connectTimeout();

    /**
     * @return timeout value for connecting to proxy in secs.
     */
    int proxyConnectTimeout();

    /**
     * @return timeout value for read from first data from
     * remote host in seconds.
     */
    int responseTimeout();

    /**
     * @return timeout value for read from subsequent data from
     * remote host in secs.
     */
    int readTimeout();

    /**
     * This function sets a timeout of @p timeout seconds and calls
     * special(data) when the timeout occurs as if it was called by the
     * application.
     *
     * A timeout can only occur when the slave is waiting for a command
     * from the application.
     *
     * Specifying a negative timeout cancels a pending timeout.
     *
     * Only one timeout at a time is supported, setting a timeout
     * cancels any pending timeout.
     */
    void setTimeoutSpecialCommand(int timeout, const QByteArray &data=QByteArray());

    /////////////////
    // Dispatching (internal)
    ////////////////

    /**
     * @internal
     */
    virtual void dispatch( int command, const QByteArray &data );

    /**
     * @internal
     */
    virtual void dispatchOpenCommand( int command, const QByteArray &data );

    /**
     * Read data sent by the job, after a dataReq
     *
     * @param buffer buffer where data is stored
     * @return 0 on end of data,
     *         > 0 bytes read
     *         < 0 error
     **/
    int readData( QByteArray &buffer );

    /**
     * internal function to be called by the slave.
     * It collects entries and emits them via listEntries
     * when enough of them are there or a certain time
     * frame exceeded (to make sure the app gets some
     * items in time but not too many items one by one
     * as this will cause a drastic performance penalty)
     * @param _entry The UDSEntry containing all of the object attributes.
     * @param ready set to true after emitting all items. @p _entry is not
     *        used in this case
     */
    void listEntry( const UDSEntry& _entry, bool ready);

    /**
     * internal function to connect a slave to/ disconnect from
     * either the slave pool or the application
     */
    void connectSlave(const QString& path);
    void disconnectSlave();

    /**
     * Prompt the user for Authorization info (login & password).
     *
     * Use this function to request authorization information from
     * the end user. You can also pass an error message which explains
     * why a previous authorization attempt failed. Here is a very
     * simple example:
     *
     * \code
     * KIO::AuthInfo authInfo;
     * if ( openPasswordDialog( authInfo ) )
     * {
     *    kDebug() << QLatin1String("User: ")
     *              << authInfo.username << endl;
     *    kDebug() << QLatin1String("Password: ")
     *              << QLatin1String("Not displayed here!") << endl;
     * }
     * \endcode
     *
     * You can also preset some values like the username, caption or
     * comment as follows:
     *
     * \code
     * KIO::AuthInfo authInfo;
     * authInfo.caption= "Acme Password Dialog";
     * authInfo.username= "Wile E. Coyote";
     * QString errorMsg = "You entered an incorrect password.";
     * if ( openPasswordDialog( authInfo, errorMsg ) )
     * {
     *    kDebug() << QLatin1String("User: ")
     *              << authInfo.username << endl;
     *    kDebug() << QLatin1String("Password: ")
     *              << QLatin1String("Not displayed here!") << endl;
     * }
     * \endcode
     *
     * \note You should consider using checkCachedAuthentication() to
     * see if the password is available in kpasswdserver before calling
     * this function.
     *
     * \note A call to this function can fail and return @p false,
     * if the UIServer could not be started for whatever reason.
     *
     * @see checkCachedAuthentication
     * @param info  See AuthInfo.
     * @param errorMsg Error message to show
     * @return      @p true if user clicks on "OK", @p false otherwsie.
     */
    bool openPasswordDialog( KIO::AuthInfo& info, const QString &errorMsg = QString() );

    /**
     * Checks for cached authentication based on parameters
     * given by @p info.
     *
     * Use this function to check if any cached password exists
     * for the URL given by @p info.  If @p AuthInfo::realmValue
     * and/or @p AuthInfo::verifyPath flag is specified, then
     * they will also be factored in determining the presence
     * of a cached password.  Note that @p Auth::url is a required
     * parameter when attempting to check for cached authorization
     * info. Here is a simple example:
     *
     * \code
     * AuthInfo info;
     * info.url = KUrl("http://www.foobar.org/foo/bar");
     * info.username = "somename";
     * info.verifyPath = true;
     * if ( !checkCachedAuthentication( info ) )
     * {
     *    if ( !openPasswordDialog(info) )
     *     ....
     * }
     * \endcode
     *
     * @param       info See AuthInfo.
     * @return      @p true if cached Authorization is found, false otherwise.
     */
    bool checkCachedAuthentication( AuthInfo& info );

    /**
     * Explicitly store authentication information. openPasswordDialog already
     * stores password information automatically, you only need to call
     * this function if you want to store authentication information that
     * is different from the information returned by openPasswordDialog.
     */
    bool cacheAuthentication( const AuthInfo& info );

    /**
     * Used by the slave to check if it can connect
     * to a given host. This should be called where the slave is ready
     * to do a ::connect() on a socket. For each call to
     * requestNetwork must exist a matching call to
     * dropNetwork, or the system will stay online until
     * KNetMgr gets closed (or the SlaveBase gets destructed)!
     *
     * If KNetMgr is not running, then this is a no-op and returns true
     *
     * @param host tells the netmgr the host the slave wants to connect
     *             to. As this could also be a proxy, we can't just take
     *             the host currenctly connected to (but that's the default
     *             value)
     *
     * @return true in theorie, the host is reachable
     *         false the system is offline and the host is in a remote network.
     */
    bool requestNetwork(const QString& host = QString());

    /**
     * Used by the slave to withdraw a connection requested by
     * requestNetwork. This function cancels the last call to
     * requestNetwork. If a client uses more than one internet
     * connection, it must use dropNetwork(host) to
     * stop each request.
     *
     * If KNetMgr is not running, then this is a no-op.
     *
     * @param host the host passed to requestNetwork
     *
     * A slave should call this function every time it disconnect from a host.
     * */
    void dropNetwork(const QString& host = QString());

    /**
     * Wait for an answer to our request, until we get @p expected1 or @p expected2
     * @return the result from readData, as well as the cmd in *pCmd if set, and the data in @p data
     */
    int waitForAnswer( int expected1, int expected2, QByteArray & data, int * pCmd = 0 );

    /**
     * Internal function to transmit meta data to the application.
     * m_outgoingMetaData will be cleared; this means that if the slave is for
     * example put on hold and picked up by a different KIO::Job later the new
     * job will not see the metadata sent before.
     * See kio/DESIGN.krun for an overview of the state
     * progression of a job/slave.
     * @warning calling this method may seriously interfere with the operation
     * of KIO which relies on the presence of some metadata at some points in time.
     * You should not use it if you are not familiar with KIO and not before
     * the slave is connected to the last job before returning to idle state.
     */
    void sendMetaData();

    /**
     * Internal function to transmit meta data to the application.
     * Like sendMetaData() but m_outgoingMetaData will not be cleared.
     * This method is mainly useful in code that runs before the slave is connected
     * to its final job.
     */
    void sendAndKeepMetaData();

    /** If your ioslave was killed by a signal, wasKilled() returns true.
     Check it regularly in lengthy functions (e.g. in get();) and return
     as fast as possible from this function if wasKilled() returns true.
     This will ensure that your slave destructor will be called correctly.
     */
    bool wasKilled() const;

    /** Internally used.
     * @internal
     */
    void setKillFlag();

protected:
    /**
     * Name of the protocol supported by this slave
     */
    QByteArray mProtocol;
    //Often used by TcpSlaveBase and unlikely to change
    MetaData mOutgoingMetaData;
    MetaData mIncomingMetaData;

    virtual void virtual_hook( int id, void* data );

private:
    void send(int cmd, const QByteArray& arr = QByteArray());
    SlaveBasePrivate* const d;
    friend class SlaveBasePrivate;
};

}

#endif
