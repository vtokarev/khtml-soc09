// -*- c++ -*-
/* This file is part of the KDE libraries
    Copyright (C) 1997, 1998 Richard Moore <rich@kde.org>
                  1998 Stephan Kulow <coolo@kde.org>
                  1998 Daniel Grana <grana@ie.iwi.unibe.ch>
                  1999,2000,2001,2002,2003 Carsten Pfeiffer <pfeiffer@kde.org>
                  2003 Clarence Dang <dang@kde.org>
                  2007 David Faure <faure@kde.org>
                  2008 Rafael Fernández López <ereslibre@kde.org>

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

#include "kfilewidget.h"

#include "kfileplacesview.h"
#include "kfileplacesmodel.h"
#include "kfilebookmarkhandler_p.h"
#include "kurlcombobox.h"
#include "kurlnavigator.h"
#include "kfilepreviewgenerator.h"
#include <config-kfile.h>

#include <kactioncollection.h>
#include <kdiroperator.h>
#include <kdirselectdialog.h>
#include <kfilefiltercombo.h>
#include <kimagefilepreview.h>
#include <kmenu.h>
#include <kmimetype.h>
#include <kpushbutton.h>
#include <krecentdocument.h>
#include <ktoolbar.h>
#include <kurlcompletion.h>
#include <kuser.h>
#include <kprotocolmanager.h>
#include <kio/job.h>
#include <kio/jobuidelegate.h>
#include <kio/netaccess.h>
#include <kio/scheduler.h>
#include <krecentdirs.h>
#include <kdebug.h>
#include <kio/kfileitemdelegate.h>

#include <QtGui/QCheckBox>
#include <QtGui/QDockWidget>
#include <QtGui/QLayout>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QSplitter>
#include <QtGui/QAbstractProxyModel>
#include <QtCore/QFSFileEngine>
#include <kshell.h>
#include <kmessagebox.h>
#include <kauthorized.h>

class KFileWidgetPrivate
{
public:
    KFileWidgetPrivate(KFileWidget *widget)
        : q(widget),
          boxLayout(0),
          placesDock(0),
          placesView(0),
          placesViewSplitter(0),
          placesViewWidth(-1),
          labeledCustomWidget(0),
          bottomCustomWidget(0),
          autoSelectExtCheckBox(0),
          operationMode(KFileWidget::Opening),
          bookmarkHandler(0),
          toolbar(0),
          locationEdit(0),
          ops(0),
          filterWidget(0),
          autoSelectExtChecked(false),
          keepLocation(false),
          hasView(false),
          hasDefaultFilter(false),
          inAccept(false),
          dummyAdded(false),
          confirmOverwrite(false),
          previewGenerator(0)
    {
    }

    ~KFileWidgetPrivate()
    {
        delete bookmarkHandler; // Should be deleted before ops!
        delete ops;
    }

    void updateLocationWhatsThis();
    void updateAutoSelectExtension();
    void initSpeedbar();
    void initGUI();
    void readConfig(KConfigGroup &configGroup);
    void writeConfig(KConfigGroup &configGroup);
    void setNonExtSelection();
    void setLocationText(const KUrl&);
    void setLocationText(const KUrl::List&);
    void appendExtension(KUrl &url);
    void updateLocationEditExtension(const QString &);
    void updateFilter();
    KUrl::List& parseSelectedUrls();
    /**
     * Parses the string "line" for files. If line doesn't contain any ", the
     * whole line will be interpreted as one file. If the number of " is odd,
     * an empty list will be returned. Otherwise, all items enclosed in " "
     * will be returned as correct urls.
     */
    KUrl::List tokenize(const QString& line) const;
    /**
     * Reads the recent used files and inserts them into the location combobox
     */
    void readRecentFiles(KConfigGroup &cg);
    /**
     * Saves the entries from the location combobox.
     */
    void saveRecentFiles(KConfigGroup &cg);
    /**
     * called when an item is highlighted/selected in multiselection mode.
     * handles setting the locationEdit.
     */
    void multiSelectionChanged();

    /**
     * Returns the absolute version of the URL specified in locationEdit.
     */
    KUrl getCompleteUrl(const QString&) const;

    /**
     * Sets the dummy entry on the history combo box. If the dummy entry
     * already exists, it is overwritten with this information.
     */
    void setDummyHistoryEntry(const QString& text, const QPixmap& icon = QPixmap(),
                              bool usePreviousPixmapIfNull = true);

    /**
     * Removes the dummy entry of the history combo box.
     */
    void removeDummyHistoryEntry();

    /**
     * Asks for overwrite confirmation using a KMessageBox and returns
     * true if the user accepts.
     *
     * @since 4.2
     */
    bool toOverwrite(const KUrl&);

    // private slots
    void _k_slotLocationChanged( const QString& );
    void _k_urlEntered( const KUrl& );
    void _k_enterUrl( const KUrl& );
    void _k_enterUrl( const QString& );
    void _k_locationAccepted( const QString& );
    void _k_slotFilterChanged();
    void _k_fileHighlighted( const KFileItem& );
    void _k_fileSelected( const KFileItem& );
    void _k_slotLoadingFinished();
    void _k_fileCompletion( const QString& );
    void _k_toggleSpeedbar( bool );
    void _k_toggleBookmarks( bool );
    void _k_iconSizeSliderChanged( int );
    void _k_slotAutoSelectExtClicked();
    void _k_placesViewSplitterMoved(int, int);
    void _k_activateUrlNavigator();

    void addToRecentDocuments();

    QString locationEditCurrentText() const;

    /**
     * KIO::NetAccess::mostLocalUrl local replacement.
     * This method won't show any progress dialogs for stating, since
     * they are very annoying when stating.
     */
    static KUrl mostLocalUrl(const KUrl &url);

    KFileWidget* q;

    // the last selected url
    KUrl url;

    // the selected filenames in multiselection mode -- FIXME
    QString filenames;

    // the name of the filename set by setSelection
    QString selection;

    // now following all kind of widgets, that I need to rebuild
    // the geometry management
    QBoxLayout *boxLayout;
    QGridLayout *lafBox;
    QVBoxLayout *vbox;

    QLabel *locationLabel;
    QWidget *opsWidget;
    QWidget *pathSpacer;

    QLabel *filterLabel;
    KUrlNavigator *urlNavigator;
    KPushButton *okButton, *cancelButton;
    QDockWidget *placesDock;
    KFilePlacesView *placesView;
    QSplitter *placesViewSplitter;
    // caches the places view width. This value will be updated when the splitter
    // is moved. This allows us to properly set a value when the dialog itself
    // is resized
    int placesViewWidth;

    QWidget *labeledCustomWidget;
    QWidget *bottomCustomWidget;

    // Automatically Select Extension stuff
    QCheckBox *autoSelectExtCheckBox;
    QString extension; // current extension for this filter

    QList<KIO::StatJob*> statJobs;

    KUrl::List urlList; //the list of selected urls

    KFileWidget::OperationMode operationMode;

    // The file class used for KRecentDirs
    QString fileClass;

    KFileBookmarkHandler *bookmarkHandler;

    KActionMenu* bookmarkButton;

    KToolBar *toolbar;
    KUrlComboBox *locationEdit;
    KDirOperator *ops;
    KFileFilterCombo *filterWidget;

    KFilePlacesModel *model;

    // whether or not the _user_ has checked the above box
    bool autoSelectExtChecked : 1;

    // indicates if the location edit should be kept or cleared when changing
    // directories
    bool keepLocation : 1;

    // the KDirOperators view is set in KFileWidget::show(), so to avoid
    // setting it again and again, we have this nice little boolean :)
    bool hasView : 1;

    bool hasDefaultFilter : 1; // necessary for the operationMode
    bool autoDirectoryFollowing : 1;
    bool inAccept : 1; // true between beginning and end of accept()
    bool dummyAdded : 1; // if the dummy item has been added. This prevents the combo from having a
                     // blank item added when loaded

    bool confirmOverwrite : 1;

    KFilePreviewGenerator *previewGenerator;
};

K_GLOBAL_STATIC(KUrl, lastDirectory) // to set the start path

static const char autocompletionWhatsThisText[] = I18N_NOOP("<qt>While typing in the text area, you may be presented "
                                                  "with possible matches. "
                                                  "This feature can be controlled by clicking with the right mouse button "
                                                  "and selecting a preferred mode from the <b>Text Completion</b> menu.")  "</qt>";

// returns true if the string contains "<a>:/" sequence, where <a> is at least 2 alpha chars
static bool containsProtocolSection( const QString& string )
{
    int len = string.length();
    static const char prot[] = ":/";
    for (int i=0; i < len;) {
        i = string.indexOf( QLatin1String(prot), i );
        if (i == -1)
            return false;
        int j=i-1;
        for (; j >= 0; j--) {
            const QChar& ch( string[j] );
            if (ch.toAscii() == 0 || !ch.isLetter())
                break;
            if (ch.isSpace() && (i-j-1) >= 2)
                return true;
        }
        if (j < 0 && i >= 2)
            return true; // at least two letters before ":/"
        i += 3; // skip : and / and one char
    }
    return false;
}

KFileWidget::KFileWidget( const KUrl& startDir, QWidget *parent )
    : QWidget(parent), KAbstractFileWidget(), d(new KFileWidgetPrivate(this))
{
    d->okButton = new KPushButton(KStandardGuiItem::ok(), this);
    d->okButton->setDefault(true);
    d->cancelButton = new KPushButton(KStandardGuiItem::cancel(), this);
    // The dialog shows them
    d->okButton->hide();
    d->cancelButton->hide();

    d->opsWidget = new QWidget(this);
    QVBoxLayout *opsWidgetLayout = new QVBoxLayout(d->opsWidget);
    opsWidgetLayout->setMargin(0);
    //d->toolbar = new KToolBar(this, true);
    d->toolbar = new KToolBar(d->opsWidget, true);
    d->toolbar->setObjectName("KFileWidget::toolbar");
    d->toolbar->setMovable(false);
    opsWidgetLayout->addWidget(d->toolbar);

    d->model = new KFilePlacesModel(this);
    d->urlNavigator = new KUrlNavigator(d->model, startDir, d->opsWidget); //d->toolbar);
    d->urlNavigator->setPlacesSelectorVisible(false);
    opsWidgetLayout->addWidget(d->urlNavigator);

    KUrl u;
    KUrlComboBox *pathCombo = d->urlNavigator->editor();
#ifdef Q_WS_WIN
    foreach( const QFileInfo &drive,QFSFileEngine::drives() )
    {
        u.setPath( drive.filePath() );
        pathCombo->addDefaultUrl(u,
                                 KIO::pixmapForUrl( u, 0, KIconLoader::Small ),
                                 i18n("Drive: %1",  u.toLocalFile()));
    }
#else
    u.setPath(QDir::rootPath());
    pathCombo->addDefaultUrl(u,
                             KIO::pixmapForUrl(u, 0, KIconLoader::Small),
                             u.toLocalFile());
#endif

    u.setPath(QDir::homePath());
    pathCombo->addDefaultUrl(u, KIO::pixmapForUrl(u, 0, KIconLoader::Small),
                             u.path(KUrl::AddTrailingSlash));

    KUrl docPath;
    docPath.setPath( KGlobalSettings::documentPath() );
    if ( (u.path(KUrl::AddTrailingSlash) != docPath.path(KUrl::AddTrailingSlash)) &&
          QDir(docPath.path(KUrl::AddTrailingSlash)).exists() )
    {
        pathCombo->addDefaultUrl( docPath,
                                  KIO::pixmapForUrl( docPath, 0, KIconLoader::Small ),
                                  docPath.path(KUrl::AddTrailingSlash));
    }

    u.setPath( KGlobalSettings::desktopPath() );
    pathCombo->addDefaultUrl(u,
                             KIO::pixmapForUrl(u, 0, KIconLoader::Small),
                             u.path(KUrl::AddTrailingSlash));

    d->url = getStartUrl( startDir, d->fileClass );

    d->ops = new KDirOperator(d->url, d->opsWidget);
    d->ops->setObjectName( "KFileWidget::ops" );
    opsWidgetLayout->addWidget(d->ops);
    connect(d->ops, SIGNAL(urlEntered(const KUrl&)),
            SLOT(_k_urlEntered(const KUrl&)));
    connect(d->ops, SIGNAL(fileHighlighted(const KFileItem &)),
            SLOT(_k_fileHighlighted(const KFileItem &)));
    connect(d->ops, SIGNAL(fileSelected(const KFileItem &)),
            SLOT(_k_fileSelected(const KFileItem &)));
    connect(d->ops, SIGNAL(finishedLoading()),
            SLOT(_k_slotLoadingFinished()));

    d->ops->setupMenu(KDirOperator::SortActions |
                   KDirOperator::FileActions |
                   KDirOperator::ViewActions);
    KActionCollection *coll = d->ops->actionCollection();

    // add nav items to the toolbar
    //
    // NOTE:  The order of the button icons here differs from that
    // found in the file manager and web browser, but has been discussed
    // and agreed upon on the kde-core-devel mailing list:
    //
    // http://lists.kde.org/?l=kde-core-devel&m=116888382514090&w=2

    coll->action( "up" )->setWhatsThis(i18n("<qt>Click this button to enter the parent folder.<br /><br />"
                                            "For instance, if the current location is file:/home/%1 clicking this "
                                            "button will take you to file:/home.</qt>",  KUser().loginName() ));

    coll->action( "back" )->setWhatsThis(i18n("Click this button to move backwards one step in the browsing history."));
    coll->action( "forward" )->setWhatsThis(i18n("Click this button to move forward one step in the browsing history."));

    coll->action( "reload" )->setWhatsThis(i18n("Click this button to reload the contents of the current location."));
    coll->action( "mkdir" )->setShortcut( QKeySequence(Qt::Key_F10) );
    coll->action( "mkdir" )->setWhatsThis(i18n("Click this button to create a new folder."));

    KAction *goToNavigatorAction = coll->addAction( "gotonavigator", this, SLOT( _k_activateUrlNavigator() ) );
    goToNavigatorAction->setShortcut( QKeySequence(Qt::CTRL + Qt::Key_L) );

    KToggleAction *showSidebarAction =
        new KToggleAction(i18n("Show Places Navigation Panel"), this);
    coll->addAction("toggleSpeedbar", showSidebarAction);
    showSidebarAction->setShortcut( QKeySequence(Qt::Key_F9) );
    connect( showSidebarAction, SIGNAL( toggled( bool ) ),
             SLOT( _k_toggleSpeedbar( bool )) );

    KToggleAction *showBookmarksAction =
        new KToggleAction(i18n("Show Bookmarks"), this);
    coll->addAction("toggleBookmarks", showBookmarksAction);
    connect( showBookmarksAction, SIGNAL( toggled( bool ) ),
             SLOT( _k_toggleBookmarks( bool )) );

    KActionMenu *menu = new KActionMenu( KIcon("configure"), i18n("Options"), this);
    coll->addAction("extra menu", menu);
    menu->setWhatsThis(i18n("<qt>This is the preferences menu for the file dialog. "
                            "Various options can be accessed from this menu including: <ul>"
                            "<li>how files are sorted in the list</li>"
                            "<li>types of view, including icon and list</li>"
                            "<li>showing of hidden files</li>"
                            "<li>the Places navigation panel</li>"
                            "<li>file previews</li>"
                            "<li>separating folders from files</li></ul></qt>"));
    menu->addAction( coll->action( "sorting menu" ));
    menu->addSeparator();
    menu->addAction(coll->action("decoration menu"));
    menu->addSeparator();
    coll->action( "short view" )->setShortcut( QKeySequence(Qt::Key_F6) );
    menu->addAction( coll->action( "short view" ));
    coll->action( "detailed view" )->setShortcut( QKeySequence(Qt::Key_F7) );
    menu->addAction( coll->action( "detailed view" ));
    menu->addSeparator();
    menu->addSeparator();
    coll->action( "show hidden" )->setShortcut( QKeySequence(Qt::Key_F8) );
    menu->addAction( coll->action( "show hidden" ));
    menu->addAction( showSidebarAction );
    menu->addAction( showBookmarksAction );
    coll->action( "preview" )->setShortcut( QKeySequence(Qt::Key_F11) );
    menu->addAction( coll->action( "preview" ));

    // support for inline previews
    KToggleAction *inlinePreview = new KToggleAction( KIcon( "preview" ),
                                                      i18n( "Show Inline Preview" ), this );
    menu->addAction( inlinePreview );

    connect( inlinePreview, SIGNAL(toggled(bool)),
             d->ops, SLOT(toggleInlinePreviews(bool)) );

    menu->setDelayed( false );
    connect( menu->menu(), SIGNAL( aboutToShow() ),
             d->ops, SLOT( updateSelectionDependentActions() ));

    QSlider *iconSizeSlider = new QSlider(this);
    iconSizeSlider->setOrientation(Qt::Horizontal);
    iconSizeSlider->setMinimum(0);
    iconSizeSlider->setMaximum(100);
    connect(iconSizeSlider, SIGNAL(valueChanged(int)),
            d->ops, SLOT(changeIconsSize(int)));
    connect(d->ops, SIGNAL(currentIconSizeChanged(int)),
            iconSizeSlider, SLOT(setValue(int)));

    KActionMenu *zoom = new KActionMenu( KIcon("zoom-fit-best"), i18n("Icon Size"), this);
    zoom->setDelayed( false );
    coll->addAction("icon size", zoom);
    KAction *sizeSlider = new KAction(this);
    sizeSlider->setDefaultWidget(iconSizeSlider);
    zoom->addAction(sizeSlider);

    QWidget *midSpacer = new QWidget(this);
    midSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    d->toolbar->addAction(coll->action("back" ));
    d->toolbar->addAction(coll->action("forward"));
    d->toolbar->addAction(coll->action("up"));
    d->toolbar->addAction(coll->action("reload"));
    d->toolbar->addWidget(midSpacer);
    d->toolbar->addAction(coll->action("mkdir"));
    d->toolbar->addAction(zoom);
    d->toolbar->addAction(menu);

    d->toolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    d->toolbar->setMovable(false);

    KUrlCompletion *pathCompletionObj = new KUrlCompletion( KUrlCompletion::DirCompletion );
    pathCombo->setCompletionObject( pathCompletionObj );
    pathCombo->setAutoDeleteCompletionObject( true );

    connect( d->urlNavigator, SIGNAL( urlChanged( const KUrl&  )),
             this,  SLOT( _k_enterUrl( const KUrl& ) ));
    connect( d->urlNavigator, SIGNAL( returnPressed() ),
             d->ops,  SLOT( setFocus() ));

    QString whatsThisText;

    // the Location label/edit
    d->locationLabel = new QLabel(i18n("&Name:"), this);
    d->locationEdit = new KUrlComboBox(KUrlComboBox::Files, true, this);
    // Properly let the dialog be resized (to smaller). Otherwise we could have
    // huge dialogs that can't be resized to smaller (it would be as big as the longest
    // item in this combo box). (ereslibre)
    d->locationEdit->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLength);
    connect( d->locationEdit, SIGNAL( editTextChanged( const QString& ) ),
             SLOT( _k_slotLocationChanged( const QString& )) );

    d->updateLocationWhatsThis();
    d->locationLabel->setBuddy(d->locationEdit);

    KUrlCompletion *fileCompletionObj = new KUrlCompletion( KUrlCompletion::FileCompletion );
    KIO::StatJob *statJob = KIO::stat( d->url.url(), KIO::HideProgressInfo );
    bool ok = KIO::NetAccess::synchronousRun( statJob, 0 );

    KUrl dirUrl;
    if (ok) {
        if (statJob->statResult().isDir()) {
            dirUrl = d->url;
            dirUrl.adjustPath( KUrl::AddTrailingSlash );
        } else {
            dirUrl = d->url.upUrl();
        }
        d->urlNavigator->setUrl( dirUrl.url() );
        fileCompletionObj->setDir( dirUrl.url() );
    } else {
        dirUrl = d->url.upUrl();
        d->urlNavigator->setUrl( dirUrl.url() );
        fileCompletionObj->setDir( dirUrl.url() );
    }
    d->locationEdit->setCompletionObject( fileCompletionObj );
    d->locationEdit->setAutoDeleteCompletionObject( true );
    connect( fileCompletionObj, SIGNAL( match( const QString& ) ),
             SLOT( _k_fileCompletion( const QString& )) );

    connect(d->locationEdit, SIGNAL( returnPressed( const QString&  )),
            this,  SLOT( _k_locationAccepted( const QString& ) ));

    // the Filter label/edit
    whatsThisText = i18n("<qt>This is the filter to apply to the file list. "
                         "File names that do not match the filter will not be shown.<p>"
                         "You may select from one of the preset filters in the "
                         "drop down menu, or you may enter a custom filter "
                         "directly into the text area.</p><p>"
                         "Wildcards such as * and ? are allowed.</p></qt>");
    d->filterLabel = new QLabel(i18n("&Filter:"), this);
    d->filterLabel->setWhatsThis(whatsThisText);
    d->filterWidget = new KFileFilterCombo(this);
    // Properly let the dialog be resized (to smaller). Otherwise we could have
    // huge dialogs that can't be resized to smaller (it would be as big as the longest
    // item in this combo box). (ereslibre)
    d->filterWidget->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLength);
    d->filterWidget->setWhatsThis(whatsThisText);
    d->filterLabel->setBuddy(d->filterWidget);
    connect(d->filterWidget, SIGNAL(filterChanged()), SLOT(_k_slotFilterChanged()));

    // the Automatically Select Extension checkbox
    // (the text, visibility etc. is set in updateAutoSelectExtension(), which is called by readConfig())
    d->autoSelectExtCheckBox = new QCheckBox (this);
    d->autoSelectExtCheckBox->setStyleSheet(QString("QCheckBox { padding-top: %1px; }").arg(KDialog::spacingHint()));
    connect(d->autoSelectExtCheckBox, SIGNAL(clicked()), SLOT(_k_slotAutoSelectExtClicked()));

    d->initGUI(); // activate GM

    // read our configuration
    KSharedConfig::Ptr config = KGlobal::config();
    KConfigGroup viewConfigGroup(config, ConfigGroup);
    d->readConfig(viewConfigGroup);

    if (!ok || !statJob->statResult().isDir()) {
        if (ok) {
            d->selection = d->url.fileName();
        }
        d->locationEdit->setUrl(d->url.fileName());
    }

    inlinePreview->setChecked(d->ops->isInlinePreviewShown());
    iconSizeSlider->setValue(d->ops->iconZoom());

    KFilePreviewGenerator *pg = d->ops->previewGenerator();
    if (pg) {
        inlinePreview->setChecked(pg->isPreviewShown());
    }

    setSelection(d->selection);
    d->locationEdit->setFocus();
}

KFileWidget::~KFileWidget()
{
    KSharedConfig::Ptr config = KGlobal::config();
    config->sync();

    delete d;
}

void KFileWidget::setLocationLabel(const QString& text)
{
    d->locationLabel->setText(text);
}

void KFileWidget::setFilter(const QString& filter)
{
    int pos = filter.indexOf('/');

    // Check for an un-escaped '/', if found
    // interpret as a MIME filter.

    if (pos > 0 && filter[pos - 1] != '\\') {
        QStringList filters = filter.split(" ", QString::SkipEmptyParts);
        setMimeFilter( filters );
        return;
    }

    // Strip the escape characters from
    // escaped '/' characters.

    QString copy (filter);
    for (pos = 0; (pos = copy.indexOf("\\/", pos)) != -1; ++pos)
        copy.remove(pos, 1);

    d->ops->clearFilter();
    d->filterWidget->setFilter(copy);
    d->ops->setNameFilter(d->filterWidget->currentFilter());
    d->hasDefaultFilter = false;
    d->filterWidget->setEditable( true );

    d->updateAutoSelectExtension ();
}

QString KFileWidget::currentFilter() const
{
    return d->filterWidget->currentFilter();
}

void KFileWidget::setMimeFilter( const QStringList& mimeTypes,
                                 const QString& defaultType )
{
    d->filterWidget->setMimeFilter( mimeTypes, defaultType );

    QStringList types = d->filterWidget->currentFilter().split(" ",QString::SkipEmptyParts); //QStringList::split(" ", d->filterWidget->currentFilter());
    types.append( QLatin1String( "inode/directory" ));
    d->ops->clearFilter();
    d->ops->setMimeFilter( types );
    d->hasDefaultFilter = !defaultType.isEmpty();
    d->filterWidget->setEditable( !d->hasDefaultFilter ||
                               d->operationMode != Saving );

    d->updateAutoSelectExtension ();
}

void KFileWidget::clearFilter()
{
    d->filterWidget->setFilter( QString() );
    d->ops->clearFilter();
    d->hasDefaultFilter = false;
    d->filterWidget->setEditable( true );

    d->updateAutoSelectExtension ();
}

QString KFileWidget::currentMimeFilter() const
{
    int i = d->filterWidget->currentIndex();
    if (d->filterWidget->showsAllTypes() && i == 0)
        return QString(); // The "all types" item has no mimetype

    return d->filterWidget->filters()[i];
}

KMimeType::Ptr KFileWidget::currentFilterMimeType()
{
    return KMimeType::mimeType( currentMimeFilter() );
}

void KFileWidget::setPreviewWidget(KPreviewWidgetBase *w) {
    d->ops->setPreviewWidget(w);
    d->ops->clearHistory();
    d->hasView = true;
}

KUrl KFileWidgetPrivate::getCompleteUrl(const QString &_url) const
{
//     kDebug(kfile_area) << "got url " << _url;

    QString url = KShell::tildeExpand(_url);
    KUrl u;

    if ( KUrl::isRelativeUrl(url) ) // only a full URL isn't relative. Even /path is.
    {
        if (!url.isEmpty() && !QDir::isRelativePath(url) ) // absolute path
            u.setPath( url );
        else
        {
            u = ops->url();
//             kDebug(kfile_area) << "ops url " << u;
            u.addPath( url ); // works for filenames and relative paths
//             kDebug(kfile_area) << "after adding path " << u;
            u.cleanPath(); // fix "dir/.."
//             kDebug(kfile_area) << "after cleaning path " << u;
        }
    }
    else // complete URL
        u = url;

//     kDebug(kfile_area) << "returning url " << u;

    return u;
}

// Called by KFileDialog
void KFileWidget::slotOk()
{
//     kDebug(kfile_area) << "slotOk\n";

    const KFileItemList items = d->ops->selectedItems();
    const QString locationEditCurrentText(KShell::tildeExpand(d->locationEditCurrentText()));
    const KUrl::List locationEditCurrentTextList(d->tokenize(locationEditCurrentText));
    KFile::Modes mode = d->ops->mode();

    // if there is nothing to do, just return from here
    if (!locationEditCurrentTextList.count()) {
        return;
    }

    // Make sure that one of the modes was provided
    Q_ASSERT((mode & KFile::File) || (mode & KFile::Directory) || (mode & KFile::Files));

    // if we are on file mode, and the list of provided files/folder is greater than one, inform
    // the user about it
    if (locationEditCurrentTextList.count() > 1) {
        if (mode & KFile::File) {
            KMessageBox::sorry(this,
                               i18n("You can only select one file"),
                               i18n("More than one file provided"));
            return;
        }
    } else if (locationEditCurrentTextList.count()) {
        // if we are on file or files mode, and we have an absolute url written by
        // the user, convert it to relative
        if (!(mode & KFile::Directory) && (!KUrl::isRelativeUrl(locationEditCurrentText) ||
                                           QDir::isAbsolutePath(locationEditCurrentText))) {
            KUrl _url(locationEditCurrentText);
            KUrl url(_url.path(KUrl::RemoveTrailingSlash));
            url.setFileName(QString());
            d->ops->setUrl(url, true);
            d->locationEdit->lineEdit()->setText(_url.fileName());
            slotOk();
            return;
        }
    }

    // if we are on local mode, make sure we haven't got a remote base url
    if ((mode & KFile::LocalOnly) && !d->mostLocalUrl(d->url).isLocalFile()) {
        KMessageBox::sorry(this,
                           i18n("You can only select local files"),
                           i18n("Remote files not accepted"));
        return;
    }

    // locationEditCurrentTextList contains absolute paths
    // this is the general loop for the File and Files mode. Obviously we know
    // that the File mode will iterate only one time here
    bool directoryMode = (mode & KFile::Directory);
    bool onlyDirectoryMode = directoryMode && !(mode & KFile::File) && !(mode & KFile::Files);
    foreach (const KUrl &url, locationEditCurrentTextList) {
        d->url = url;
        KIO::StatJob *statJob = KIO::stat(url, KIO::HideProgressInfo);
        bool res = KIO::NetAccess::synchronousRun(statJob, 0);

        if (!KAuthorized::authorizeUrlAction("open", KUrl(), url)) {
            QString msg = KIO::buildErrorString(KIO::ERR_ACCESS_DENIED, d->url.prettyUrl());
            KMessageBox::error(this, msg);
            return;
        }

        if ((d->operationMode == Saving) && d->confirmOverwrite && !d->toOverwrite(url)) {
            return;
        }

        // if we are given a folder when not on directory mode, let's get into it
        if (res && !directoryMode && statJob->statResult().isDir()) {
            d->ops->setUrl(url, true);
            d->locationEdit->lineEdit()->setText(QString());
            return;
        } else if (!(mode & KFile::ExistingOnly) || res) {
            // if we don't care about ExistingOnly flag, add the file even if
            // it doesn't exist. If we care about it, don't add it to the list
            if (!onlyDirectoryMode || (res && statJob->statResult().isDir())) {
                d->urlList << url;
            }
        }
    }

    // if we have reached this point and we didn't return before, that is because
    // we want this dialog to be accepted
    emit accepted();
}

void KFileWidget::accept()
{
    d->inAccept = true; // parseSelectedUrls() checks that

    *lastDirectory = d->ops->url();
    if (!d->fileClass.isEmpty())
       KRecentDirs::add(d->fileClass, d->ops->url().url());

    // clear the topmost item, we insert it as full path later on as item 1
    d->locationEdit->setItemText( 0, QString() );

    KUrl::List list = selectedUrls();
    QList<KUrl>::const_iterator it = list.begin();
    int atmost = d->locationEdit->maxItems(); //don't add more items than necessary
    for ( ; it != list.end() && atmost > 0; ++it ) {
        const KUrl& url = *it;
        // we strip the last slash (-1) because KUrlComboBox does that as well
        // when operating in file-mode. If we wouldn't , dupe-finding wouldn't
        // work.
        QString file = url.isLocalFile() ? url.path(KUrl::RemoveTrailingSlash) : url.prettyUrl(KUrl::RemoveTrailingSlash);

        // remove dupes
        for ( int i = 1; i < d->locationEdit->count(); i++ ) {
            if ( d->locationEdit->itemText( i ) == file ) {
                d->locationEdit->removeItem( i-- );
                break;
            }
        }
        //FIXME I don't think this works correctly when the KUrlComboBox has some default urls.
        //KUrlComboBox should provide a function to add an url and rotate the existing ones, keeping
        //track of maxItems, and we shouldn't be able to insert items as we please.
        d->locationEdit->insertItem( 1,file);
        atmost--;
    }

    KSharedConfig::Ptr config = KGlobal::config();
    KConfigGroup grp(config,ConfigGroup);
    d->writeConfig(grp);
    d->saveRecentFiles(grp);

    d->addToRecentDocuments();

    if (!(mode() & KFile::Files)) { // single selection
        emit fileSelected(d->url.url());
    }

    d->ops->close();
}


void KFileWidgetPrivate::_k_fileHighlighted(const KFileItem &i)
{
    const bool modified = locationEdit->lineEdit()->isModified();
    locationEdit->lineEdit()->setModified( false );

    if ((!i.isNull() && i.isDir() ) ||
        (locationEdit->hasFocus() && !locationEdit->currentText().isEmpty())) // don't disturb
        return;

    if (!(ops->mode() & KFile::Files)) {
        if (i.isNull()) {
            if (!modified) {
                setLocationText(KUrl());
            }
            return;
        }

        url = i.url();

        if (!locationEdit->hasFocus()) { // don't disturb while editing
            setLocationText( url );
        }

        emit q->fileHighlighted(url.url());
    } else {
        multiSelectionChanged();
        emit q->selectionChanged();
    }

    locationEdit->lineEdit()->selectAll();
}

void KFileWidgetPrivate::_k_fileSelected(const KFileItem &i)
{
    if (!i.isNull() && i.isDir()) {
        return;
    }

    if (!(ops->mode() & KFile::Files)) {
        if (i.isNull()) {
            setLocationText(KUrl());
            return;
        }
        setLocationText(i.url());
    } else {
        multiSelectionChanged();
        emit q->selectionChanged();
    }

    q->slotOk();
}


// I know it's slow to always iterate thru the whole filelist
// (d->ops->selectedItems()), but what can we do?
void KFileWidgetPrivate::multiSelectionChanged()
{
    if (locationEdit->hasFocus() && !locationEdit->currentText().isEmpty()) { // don't disturb
        return;
    }

    const KFileItemList list = ops->selectedItems();

    if (list.isEmpty()) {
        setLocationText(KUrl());
        return;
    }

    KUrl::List urlList;
    foreach (const KFileItem &fileItem, list) {
        urlList << fileItem.url();
    }

    setLocationText(urlList);
}

void KFileWidgetPrivate::setDummyHistoryEntry( const QString& text, const QPixmap& icon,
                                               bool usePreviousPixmapIfNull )
{
    // setCurrentItem() will cause textChanged() being emitted,
    // so slotLocationChanged() will be called. Make sure we don't clear
    // the KDirOperator's view-selection in there
    QObject::disconnect( locationEdit, SIGNAL( editTextChanged( const QString& ) ),
                        q, SLOT( _k_slotLocationChanged( const QString& ) ) );

    bool dummyExists = dummyAdded;

    int cursorPosition = locationEdit->lineEdit()->cursorPosition();

    if ( dummyAdded ) {
        if ( !icon.isNull() ) {
            locationEdit->setItemIcon( 0, icon );
            locationEdit->setItemText( 0, text );
        } else {
            if ( !usePreviousPixmapIfNull ) {
                locationEdit->setItemIcon( 0, QPixmap() );
            }
            locationEdit->setItemText( 0, text );
        }
    } else {
        if ( !text.isEmpty() ) {
            if ( !icon.isNull() ) {
                locationEdit->insertItem( 0, icon, text );
            } else {
                if ( !usePreviousPixmapIfNull ) {
                    locationEdit->insertItem( 0, QPixmap(), text );
                } else {
                    locationEdit->insertItem( 0, text );
                }
            }
            dummyAdded = true;
            dummyExists = true;
        }
    }

    if ( dummyExists && !text.isEmpty() ) {
        locationEdit->setCurrentIndex( 0 );
    }

    locationEdit->lineEdit()->setCursorPosition( cursorPosition );

    QObject::connect( locationEdit, SIGNAL( editTextChanged ( const QString& ) ),
                    q, SLOT( _k_slotLocationChanged( const QString& )) );
}

void KFileWidgetPrivate::removeDummyHistoryEntry()
{
    if ( !dummyAdded ) {
        return;
    }

    // setCurrentItem() will cause textChanged() being emitted,
    // so slotLocationChanged() will be called. Make sure we don't clear
    // the KDirOperator's view-selection in there
    QObject::disconnect( locationEdit, SIGNAL( editTextChanged( const QString& ) ),
                        q, SLOT( _k_slotLocationChanged( const QString& ) ) );

    locationEdit->removeItem( 0 );
    locationEdit->setCurrentIndex( -1 );
    dummyAdded = false;

    QObject::connect( locationEdit, SIGNAL( editTextChanged ( const QString& ) ),
                    q, SLOT( _k_slotLocationChanged( const QString& )) );
}

void KFileWidgetPrivate::setLocationText(const KUrl& url)
{
    if (!url.isEmpty()) {
        QPixmap mimeTypeIcon = KIconLoader::global()->loadMimeTypeIcon( KMimeType::iconNameForUrl( url ), KIconLoader::Small );
        setDummyHistoryEntry( url.fileName(), mimeTypeIcon );
    } else {
        removeDummyHistoryEntry();
    }

    // don't change selection when user has clicked on an item
    if (operationMode == KFileWidget::Saving && !locationEdit->isVisible()) {
       setNonExtSelection();
    }
}

void KFileWidgetPrivate::setLocationText( const KUrl::List& urlList )
{
    if ( urlList.count() > 1 ) {
        QString urls;
        foreach (const KUrl &url, urlList) {
            urls += QString( "\"%1\"" ).arg( url.fileName() ) + ' ';
        }
        urls = urls.left( urls.size() - 1 );

        setDummyHistoryEntry( urls, QPixmap(), false );
    } else if ( urlList.count() ) {
        const QPixmap mimeTypeIcon = KIconLoader::global()->loadMimeTypeIcon( KMimeType::iconNameForUrl( urlList[0] ),  KIconLoader::Small );
        setDummyHistoryEntry( urlList[0].fileName(), mimeTypeIcon );
    } else {
        removeDummyHistoryEntry();
    }

    // don't change selection when user has clicked on an item
    if ( operationMode == KFileWidget::Saving && !locationEdit->isVisible())
       setNonExtSelection();
}

void KFileWidgetPrivate::updateLocationWhatsThis()
{
    QString whatsThisText;
    if (operationMode == KFileWidget::Saving)
    {
        whatsThisText = "<qt>" + i18n("This is the name to save the file as.") +
                             i18n (autocompletionWhatsThisText);
    }
    else if (ops->mode() & KFile::Files)
    {
        whatsThisText = "<qt>" + i18n("This is the list of files to open. More than "
                             "one file can be specified by listing several "
                             "files, separated by spaces.") +
                              i18n (autocompletionWhatsThisText);
    }
    else
    {
        whatsThisText = "<qt>" + i18n("This is the name of the file to open.") +
                             i18n (autocompletionWhatsThisText);
    }

    locationLabel->setWhatsThis(whatsThisText);
    locationEdit->setWhatsThis(whatsThisText);
}

void KFileWidgetPrivate::initSpeedbar()
{
    if (placesDock) {
        return;
    }

    placesDock = new QDockWidget(i18n("Places"), q);
    placesDock->setFeatures(QDockWidget::DockWidgetClosable);

    placesView = new KFilePlacesView(placesDock);
    placesView->setModel(model);
    placesView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    placesView->setObjectName(QLatin1String("url bar"));
    QObject::connect(placesView, SIGNAL(urlChanged(KUrl)),
                     q, SLOT(_k_enterUrl(KUrl)));

    // need to set the current url of the urlbar manually (not via urlEntered()
    // here, because the initial url of KDirOperator might be the same as the
    // one that will be set later (and then urlEntered() won't be emitted).
    // TODO: KDE5 ### REMOVE THIS when KDirOperator's initial URL (in the c'tor) is gone.
    placesView->setUrl(url);

    placesDock->setWidget(placesView);
    placesViewSplitter->insertWidget(0, placesDock);

    // initialize the size of the splitter
    KConfigGroup configGroup(KGlobal::config(), ConfigGroup);
    placesViewWidth = configGroup.readEntry(SpeedbarWidth, 0);

    QList<int> sizes = placesViewSplitter->sizes();
    if (placesViewWidth > 0) {
        sizes[0] = placesViewWidth + 1;
        sizes[1] = q->width() - placesViewWidth -1;
        placesViewSplitter->setSizes(sizes);
    }

    QObject::connect(placesDock, SIGNAL(visibilityChanged(bool)),
                     q, SLOT(_k_toggleSpeedbar(bool)));
}

void KFileWidgetPrivate::initGUI()
{
    delete boxLayout; // deletes all sub layouts

    boxLayout = new QVBoxLayout( q);
    boxLayout->setMargin(0); // no additional margin to the already existing
    boxLayout->setSpacing(0);

    placesViewSplitter = new QSplitter(q);
    placesViewSplitter->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    placesViewSplitter->setChildrenCollapsible(false);
    boxLayout->addWidget(placesViewSplitter);

    QObject::connect(placesViewSplitter, SIGNAL(splitterMoved(int,int)),
                     q, SLOT(_k_placesViewSplitterMoved(int,int)));
    placesViewSplitter->insertWidget(0, opsWidget);

    vbox = new QVBoxLayout();
    vbox->setMargin(0);
    vbox->addSpacing(KDialog::spacingHint());
    boxLayout->addLayout(vbox);

    lafBox = new QGridLayout();

    lafBox->setSpacing(KDialog::spacingHint());
    lafBox->addWidget(locationLabel, 0, 0, Qt::AlignVCenter | Qt::AlignRight);
    lafBox->addWidget(locationEdit, 0, 1, Qt::AlignVCenter);
    lafBox->addWidget(okButton, 0, 2, Qt::AlignVCenter);

    lafBox->addWidget(filterLabel, 1, 0, Qt::AlignVCenter | Qt::AlignRight);
    lafBox->addWidget(filterWidget, 1, 1, Qt::AlignVCenter);
    lafBox->addWidget(cancelButton, 1, 2, Qt::AlignVCenter);

    lafBox->setColumnStretch(1, 4);

    vbox->addLayout(lafBox);

    // add the Automatically Select Extension checkbox
    vbox->addWidget(autoSelectExtCheckBox);

    q->setTabOrder(ops, autoSelectExtCheckBox);
    q->setTabOrder(autoSelectExtCheckBox, locationEdit);
    q->setTabOrder(locationEdit, filterWidget);
    q->setTabOrder(filterWidget, okButton);
    q->setTabOrder(okButton, cancelButton);
    q->setTabOrder(cancelButton, urlNavigator);
    q->setTabOrder(urlNavigator, ops);
    q->setTabOrder(cancelButton, urlNavigator);
    q->setTabOrder(urlNavigator, ops);

}

void KFileWidgetPrivate::_k_slotFilterChanged()
{
//     kDebug(kfile_area);

    QString filter = filterWidget->currentFilter();
    ops->clearFilter();

    if ( filter.indexOf( '/' ) > -1 ) {
        QStringList types = filter.split(' ', QString::SkipEmptyParts);
        types.prepend("inode/directory");
        ops->setMimeFilter( types );
    }
    else
        ops->setNameFilter( filter );

    ops->updateDir();

    updateAutoSelectExtension();

    emit q->filterChanged(filter);
}


void KFileWidget::setUrl(const KUrl& url, bool clearforward)
{
//     kDebug(kfile_area) << "setting url " << url;

    d->selection.clear();
    d->ops->setUrl(url, clearforward);
}

// Protected
void KFileWidgetPrivate::_k_urlEntered(const KUrl& url)
{
//     kDebug(kfile_area);

    QString filename = locationEditCurrentText();
    selection.clear();

    KUrlComboBox* pathCombo = urlNavigator->editor();
    if ( pathCombo->count() != 0 ) { // little hack
        pathCombo->setUrl( url );
    }

    bool blocked = locationEdit->blockSignals(true);
    if (keepLocation) {
        locationEdit->changeUrl(0, KIcon(KMimeType::iconNameForUrl(filename)), filename);
        locationEdit->lineEdit()->setModified(true);
    }

    locationEdit->blockSignals( blocked );

    urlNavigator->setUrl(  url );

    QString dir = url.url(KUrl::AddTrailingSlash);
    // is trigged in ctor before completion object is set
    KUrlCompletion *completion = dynamic_cast<KUrlCompletion*>( locationEdit->completionObject() );
    if( completion )
        completion->setDir( dir );

    if (placesView) {
        placesView->setUrl( url );
    }
}

void KFileWidgetPrivate::_k_locationAccepted(const QString &url)
{
    Q_UNUSED(url);
//     kDebug(kfile_area);
    q->slotOk();
}

void KFileWidgetPrivate::_k_enterUrl( const KUrl& url )
{
//     kDebug(kfile_area);

    KUrl fixedUrl( url );
    // append '/' if needed: url combo does not add it
    // tokenize() expects it because uses KUrl::setFileName()
    fixedUrl.adjustPath( KUrl::AddTrailingSlash );
    q->setUrl( fixedUrl );
    if (!locationEdit->hasFocus())
        ops->setFocus();
}

void KFileWidgetPrivate::_k_enterUrl( const QString& url )
{
//     kDebug(kfile_area);

    _k_enterUrl( KUrl( KUrlCompletion::replacedPath( url, true, true )) );
}

bool KFileWidgetPrivate::toOverwrite(const KUrl &url)
{
//     kDebug(kfile_area);

    KIO::StatJob *statJob = KIO::stat(url, KIO::HideProgressInfo);
    bool res = KIO::NetAccess::synchronousRun(statJob, 0);

    if (res) {
        int ret = KMessageBox::warningContinueCancel( q,
            i18n( "The file \"%1\" already exists. Do you wish to overwrite it?" ,
            url.fileName() ), i18n( "Overwrite File?" ), KStandardGuiItem::overwrite());

        if (ret != KMessageBox::Continue) {
            return false;
        }
        return true;
    }

    return true;
}

void KFileWidget::setSelection(const QString& url)
{
//     kDebug(kfile_area) << "setSelection " << url;

    if (url.isEmpty()) {
        d->selection.clear();
        return;
    }

    KUrl u = d->getCompleteUrl(url);
    if (!u.isValid()) { // if it still is
        kWarning() << url << " is not a correct argument for setSelection!";
        return;
    }

    // Honor protocols that do not support directory listing
    if (!KProtocolManager::supportsListing(u))
        return;

    d->setLocationText(url);
}

void KFileWidgetPrivate::_k_slotLoadingFinished()
{
//     kDebug(kfile_area);

    if ( !selection.isEmpty() )
        ops->setCurrentItem( selection );
}

void KFileWidgetPrivate::_k_fileCompletion( const QString& match )
{
//     kDebug(kfile_area);

    if (locationEdit->currentText().contains('"')) {
        return;
    }

    if (match.isEmpty() && ops->view()) {
        ops->view()->clearSelection();
    } else {
        ops->setCurrentItem(match);
        setDummyHistoryEntry(locationEdit->currentText(), KIconLoader::global()->loadMimeTypeIcon( KMimeType::iconNameForUrl( match ), KIconLoader::Small), !locationEdit->currentText().isEmpty());
        locationEdit->setCompletedText(match);
    }
}

void KFileWidgetPrivate::_k_slotLocationChanged( const QString& text )
{
//     kDebug(kfile_area);

    locationEdit->lineEdit()->setModified(true);

    if (text.isEmpty() && ops->view()) {
        ops->view()->clearSelection();
    }

    if (text.isEmpty()) {
        removeDummyHistoryEntry();
    } else {
        setDummyHistoryEntry( text );
    }

    if (!locationEdit->lineEdit()->text().isEmpty()) {
        const KUrl::List urlList(tokenize(text));
        QStringList stringList;
        foreach (const KUrl &url, urlList) {
            stringList << url.fileName();
        }
        ops->setCurrentItems(stringList);
    }

    updateFilter();
}

KUrl KFileWidget::selectedUrl() const
{
//     kDebug(kfile_area);

    if ( d->inAccept )
        return d->url;
    else
        return KUrl();
}

KUrl::List KFileWidget::selectedUrls() const
{
//     kDebug(kfile_area);

    KUrl::List list;
    if ( d->inAccept ) {
        if (d->ops->mode() & KFile::Files)
            list = d->parseSelectedUrls();
        else
            list.append( d->url );
    }
    return list;
}


KUrl::List& KFileWidgetPrivate::parseSelectedUrls()
{
//     kDebug(kfile_area);

    if ( filenames.isEmpty() ) {
        return urlList;
    }

    urlList.clear();
    if ( filenames.contains( '/' )) { // assume _one_ absolute filename
        KUrl u;
        if ( containsProtocolSection( filenames ) )
            u = filenames;
        else
            u.setPath( filenames );

        if ( u.isValid() )
            urlList.append( u );
        else
            KMessageBox::error( q,
                                i18n("The chosen filenames do not\n"
                                     "appear to be valid."),
                                i18n("Invalid Filenames") );
    }

    else
        urlList = tokenize( filenames );

    filenames.clear(); // indicate that we parsed that one

    return urlList;
}


// FIXME: current implementation drawback: a filename can't contain quotes
KUrl::List KFileWidgetPrivate::tokenize( const QString& line ) const
{
//     kDebug(kfile_area);

    KUrl::List urls;
    KUrl u( ops->url() );
    QString name;

    const int count = line.count( QLatin1Char( '"' ) );
    if ( count == 0 ) { // no " " -> assume one single file
        u.setFileName( line );
        if ( u.isValid() )
            urls.append( u );

        return urls;
    }

    int start = 0;
    int index1 = -1, index2 = -1;
    while ( true ) {
        index1 = line.indexOf( '"', start );
        index2 = line.indexOf( '"', index1 + 1 );

        if ( index1 < 0 || index2 < 0 )
            break;

        // get everything between the " "
        name = line.mid( index1 + 1, index2 - index1 - 1 );
        u.setFileName( name );
        if ( u.isValid() )
            urls.append( u );

        start = index2 + 1;
    }
    return urls;
}


QString KFileWidget::selectedFile() const
{
//     kDebug(kfile_area);

    if ( d->inAccept ) {
        const KUrl url = d->mostLocalUrl(d->url);
        if (url.isLocalFile())
            return url.path();
        else {
            KMessageBox::sorry( const_cast<KFileWidget*>(this),
                                i18n("You can only select local files."),
                                i18n("Remote Files Not Accepted") );
        }
    }
    return QString();
}

QStringList KFileWidget::selectedFiles() const
{
//     kDebug(kfile_area);

    QStringList list;

    if (d->inAccept) {
        if (d->ops->mode() & KFile::Files) {
            KUrl::List urls = d->parseSelectedUrls();
            QList<KUrl>::const_iterator it = urls.begin();
            while (it != urls.end()) {
                KUrl url = d->mostLocalUrl(*it);
                if (url.isLocalFile())
                    list.append(url.path());
                ++it;
            }
        }

        else { // single-selection mode
            if ( d->url.isLocalFile() )
                list.append( d->url.path() );
        }
    }

    return list;
}

KUrl KFileWidget::baseUrl() const
{
    return d->ops->url();
}

void KFileWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);

    if (d->placesDock) {
        // we don't want our places dock actually changing size when we resize
        // and qt doesn't make it easy to enforce such a thing with QSplitter
        QList<int> sizes = d->placesViewSplitter->sizes();
        sizes[0] = d->placesViewWidth + 1; // without this pixel, our places view is reduced 1 pixel each time is shown.
        sizes[1] = width() - d->placesViewWidth - 1;
        d->placesViewSplitter->setSizes( sizes );
    }
}

void KFileWidget::showEvent(QShowEvent* event)
{
    if ( !d->hasView ) { // delayed view-creation
        d->ops->setView( KFile::Default );
        d->ops->view()->setSizePolicy( QSizePolicy( QSizePolicy::Maximum, QSizePolicy::Maximum ) );
        d->hasView = true;
    }
    d->ops->clearHistory();

    QWidget::showEvent(event);
}

void KFileWidget::setMode( KFile::Modes m )
{
//     kDebug(kfile_area);

    d->ops->setMode(m);
    if ( d->ops->dirOnlyMode() ) {
        d->filterWidget->setDefaultFilter( i18n("*|All Folders") );
    }
    else {
        d->filterWidget->setDefaultFilter( i18n("*|All Files") );
    }

    d->updateAutoSelectExtension();
}

KFile::Modes KFileWidget::mode() const
{
    return d->ops->mode();
}


void KFileWidgetPrivate::readConfig(KConfigGroup &configGroup)
{
//     kDebug(kfile_area);

    readRecentFiles(configGroup);

    ops->setViewConfig(configGroup);
    ops->readConfig(configGroup);

    KUrlComboBox *combo = urlNavigator->editor();
    combo->setUrls( configGroup.readPathEntry( RecentURLs, QStringList() ), KUrlComboBox::RemoveTop );
    combo->setMaxItems( configGroup.readEntry( RecentURLsNumber,
                                       DefaultRecentURLsNumber ) );
    combo->setUrl( ops->url() );
    autoDirectoryFollowing = configGroup.readEntry(AutoDirectoryFollowing,
                                                   DefaultDirectoryFollowing);

    KGlobalSettings::Completion cm = (KGlobalSettings::Completion)
                                      configGroup.readEntry( PathComboCompletionMode,
                                      static_cast<int>( KGlobalSettings::completionMode() ) );
    if ( cm != KGlobalSettings::completionMode() )
        combo->setCompletionMode( cm );

    cm = (KGlobalSettings::Completion)
         configGroup.readEntry( LocationComboCompletionMode,
                        static_cast<int>( KGlobalSettings::completionMode() ) );
    if ( cm != KGlobalSettings::completionMode() )
        locationEdit->setCompletionMode( cm );

    // show or don't show the speedbar
    _k_toggleSpeedbar( configGroup.readEntry( ShowSpeedbar, true ) );

    // show or don't show the bookmarks
    _k_toggleBookmarks( configGroup.readEntry(ShowBookmarks, false) );

    // does the user want Automatically Select Extension?
    autoSelectExtChecked = configGroup.readEntry (AutoSelectExtChecked, DefaultAutoSelectExtChecked);
    updateAutoSelectExtension();

    // should the URL navigator use the breadcrumb navigation?
    urlNavigator->setUrlEditable( !configGroup.readEntry(BreadcrumbNavigation, true) );

    // should the URL navigator show the full path?
    urlNavigator->setShowFullPath( configGroup.readEntry(ShowFullPath, false) );

    int w1 = q->minimumSize().width();
    int w2 = toolbar->sizeHint().width();
    if (w1 < w2)
        q->setMinimumWidth(w2);
}

void KFileWidgetPrivate::writeConfig(KConfigGroup &configGroup)
{
//     kDebug(kfile_area);

    // these settings are global settings; ALL instances of the file dialog
    // should reflect them
    configGroup.config()->setForceGlobal(true);

    KUrlComboBox *pathCombo = urlNavigator->editor();
    configGroup.writePathEntry( RecentURLs, pathCombo->urls() );
    //saveDialogSize( configGroup, KConfigGroup::Persistent | KConfigGroup::Global );
    configGroup.writeEntry( PathComboCompletionMode, static_cast<int>(pathCombo->completionMode()) );
    configGroup.writeEntry( LocationComboCompletionMode, static_cast<int>(locationEdit->completionMode()) );

    const bool showSpeedbar = placesDock && !placesDock->isHidden();
    configGroup.writeEntry( ShowSpeedbar, showSpeedbar );
    if (showSpeedbar) {
        const QList<int> sizes = placesViewSplitter->sizes();
        Q_ASSERT( sizes.count() > 0 );
        configGroup.writeEntry( SpeedbarWidth, sizes[0] );
    }

    configGroup.writeEntry( ShowBookmarks, bookmarkHandler != 0 );
    configGroup.writeEntry( AutoSelectExtChecked, autoSelectExtChecked );
    configGroup.writeEntry( BreadcrumbNavigation, !urlNavigator->isUrlEditable() );
    configGroup.writeEntry( ShowFullPath, urlNavigator->showFullPath() );

    ops->writeConfig(configGroup);
    configGroup.config()->setForceGlobal(false);
}


void KFileWidgetPrivate::readRecentFiles(KConfigGroup &cg)
{
//     kDebug(kfile_area);

    locationEdit->setMaxItems(cg.readEntry(RecentFilesNumber, DefaultRecentURLsNumber));
    locationEdit->setUrls(cg.readPathEntry(RecentFiles, QStringList()),
                          KUrlComboBox::RemoveBottom);
    locationEdit->setCurrentIndex(-1);
}

void KFileWidgetPrivate::saveRecentFiles(KConfigGroup &cg)
{
//     kDebug(kfile_area);
    cg.writePathEntry(RecentFiles, locationEdit->urls());
}

KPushButton * KFileWidget::okButton() const
{
    return d->okButton;
}

KPushButton * KFileWidget::cancelButton() const
{
    return d->cancelButton;
}

// Called by KFileDialog
void KFileWidget::slotCancel()
{
//     kDebug(kfile_area);

    d->ops->close();

    KConfigGroup grp(KGlobal::config(), ConfigGroup);
    d->writeConfig(grp);
}

void KFileWidget::setKeepLocation( bool keep )
{
    d->keepLocation = keep;
}

bool KFileWidget::keepsLocation() const
{
    return d->keepLocation;
}

void KFileWidget::setOperationMode( OperationMode mode )
{
//     kDebug(kfile_area);

    d->operationMode = mode;
    d->keepLocation = (mode == Saving);
    d->filterWidget->setEditable( !d->hasDefaultFilter || mode != Saving );
    if ( mode == Opening )
       // don't use KStandardGuiItem::open() here which has trailing ellipsis!
       d->okButton->setGuiItem( KGuiItem( i18n( "&Open" ), "document-open") );
    else if ( mode == Saving ) {
       d->okButton->setGuiItem( KStandardGuiItem::save() );
       d->setNonExtSelection();
    }
    else
       d->okButton->setGuiItem( KStandardGuiItem::ok() );
    d->updateLocationWhatsThis();
    d->updateAutoSelectExtension();
}

KFileWidget::OperationMode KFileWidget::operationMode() const
{
    return d->operationMode;
}

void KFileWidgetPrivate::_k_slotAutoSelectExtClicked()
{
//     kDebug (kfile_area) << "slotAutoSelectExtClicked(): "
//                          << autoSelectExtCheckBox->isChecked() << endl;

    // whether the _user_ wants it on/off
    autoSelectExtChecked = autoSelectExtCheckBox->isChecked();

    // update the current filename's extension
    updateLocationEditExtension (extension /* extension hasn't changed */);
}

void KFileWidgetPrivate::_k_placesViewSplitterMoved(int pos, int index)
{
//     kDebug(kfile_area);

    // we need to record the size of the splitter when the splitter changes size
    // so we can keep the places box the right size!
    if (placesDock && index == 1) {
        placesViewWidth = pos;
//         kDebug() << "setting lafBox minwidth to" << placesViewWidth;
        lafBox->setColumnMinimumWidth(0, placesViewWidth);
    }
}

void KFileWidgetPrivate::_k_activateUrlNavigator()
{
//     kDebug(kfile_area);

    urlNavigator->setUrlEditable(true);
    urlNavigator->setFocus();
    urlNavigator->editor()->lineEdit()->selectAll();
}

static QString getExtensionFromPatternList(const QStringList &patternList)
{
//     kDebug(kfile_area);

    QString ret;
//     kDebug (kfile_area) << "\tgetExtension " << patternList;

    QStringList::ConstIterator patternListEnd = patternList.end();
    for (QStringList::ConstIterator it = patternList.begin();
         it != patternListEnd;
         ++it)
    {
//         kDebug (kfile_area) << "\t\ttry: \'" << (*it) << "\'";

        // is this pattern like "*.BMP" rather than useless things like:
        //
        // README
        // *.
        // *.*
        // *.JP*G
        // *.JP?
        if ((*it).startsWith ("*.") &&
            (*it).length() > 2 &&
            (*it).indexOf('*', 2) < 0 && (*it).indexOf ('?', 2) < 0)
        {
            ret = (*it).mid (1);
            break;
        }
    }

    return ret;
}

static QString stripUndisplayable (const QString &string)
{
    QString ret = string;

    ret.remove (':');
    ret.remove ('&');

    return ret;
}


//QString KFileWidget::currentFilterExtension()
//{
//    return d->extension;
//}

void KFileWidgetPrivate::updateAutoSelectExtension()
{
    if (!autoSelectExtCheckBox) return;

    //
    // Figure out an extension for the Automatically Select Extension thing
    // (some Windows users apparently don't know what to do when confronted
    // with a text file called "COPYING" but do know what to do with
    // COPYING.txt ...)
    //

//     kDebug (kfile_area) << "Figure out an extension: ";
    QString lastExtension = extension;
    extension.clear();

    // Automatically Select Extension is only valid if the user is _saving_ a _file_
    if ((operationMode == KFileWidget::Saving) && (ops->mode() & KFile::File))
    {
        //
        // Get an extension from the filter
        //

        QString filter = filterWidget->currentFilter();
        if (!filter.isEmpty())
        {
            // e.g. "*.cpp"
            if (filter.indexOf ('/') < 0)
            {
                extension = getExtensionFromPatternList (filter.split(" ",QString::SkipEmptyParts)/*QStringList::split (" ", filter)*/).toLower();
//                 kDebug (kfile_area) << "\tsetFilter-style: pattern ext=\'"
//                                     << extension << "\'" << endl;
            }
            // e.g. "text/html"
            else
            {
                KMimeType::Ptr mime = KMimeType::mimeType (filter);

                if (mime)
                {
                    // first try X-KDE-NativeExtension
                    QString nativeExtension = mime->property ("X-KDE-NativeExtension").toString();
                    if (!nativeExtension.isEmpty() && nativeExtension.at (0) == '.')
                    {
                        extension = nativeExtension.toLower();
//                         kDebug (kfile_area) << "\tsetMimeFilter-style: native ext=\'"
//                                             << extension << "\'" << endl;
                    }

                    // no X-KDE-NativeExtension
                    if (extension.isEmpty())
                    {
                        extension = getExtensionFromPatternList (mime->patterns()).toLower();
//                         kDebug (kfile_area) << "\tsetMimeFilter-style: pattern ext=\'"
//                                             << extension << "\'" << endl;
                    }
                }
            }
        }


        //
        // GUI: checkbox
        //

        QString whatsThisExtension;
        if (!extension.isEmpty())
        {
            // remember: sync any changes to the string with below
            autoSelectExtCheckBox->setText (i18n ("Automatically select filename e&xtension (%1)",  extension));
            whatsThisExtension = i18n ("the extension <b>%1</b>",  extension);

            autoSelectExtCheckBox->setEnabled (true);
            autoSelectExtCheckBox->setChecked (autoSelectExtChecked);
        }
        else
        {
            // remember: sync any changes to the string with above
            autoSelectExtCheckBox->setText (i18n ("Automatically select filename e&xtension"));
            whatsThisExtension = i18n ("a suitable extension");

            autoSelectExtCheckBox->setChecked (false);
            autoSelectExtCheckBox->setEnabled (false);
        }

        const QString locationLabelText = stripUndisplayable (locationLabel->text());
        const QString filterLabelText = stripUndisplayable (filterLabel->text());
        autoSelectExtCheckBox->setWhatsThis(            "<qt>" +
                i18n (
                  "This option enables some convenient features for "
                  "saving files with extensions:<br />"
                  "<ol>"
                    "<li>Any extension specified in the <b>%1</b> text "
                    "area will be updated if you change the file type "
                    "to save in.<br />"
                    "<br /></li>"
                    "<li>If no extension is specified in the <b>%2</b> "
                    "text area when you click "
                    "<b>Save</b>, %3 will be added to the end of the "
                    "filename (if the filename does not already exist). "
                    "This extension is based on the file type that you "
                    "have chosen to save in.<br />"
                    "<br />"
                    "If you do not want KDE to supply an extension for the "
                    "filename, you can either turn this option off or you "
                    "can suppress it by adding a period (.) to the end of "
                    "the filename (the period will be automatically "
                    "removed)."
                    "</li>"
                  "</ol>"
                  "If unsure, keep this option enabled as it makes your "
                  "files more manageable."
                    ,
                  locationLabelText,
                  locationLabelText,
                  whatsThisExtension)
            + "</qt>"
            );

        autoSelectExtCheckBox->show();


        // update the current filename's extension
        updateLocationEditExtension (lastExtension);
    }
    // Automatically Select Extension not valid
    else
    {
        autoSelectExtCheckBox->setChecked (false);
        autoSelectExtCheckBox->hide();
    }
}

// Updates the extension of the filename specified in d->locationEdit if the
// Automatically Select Extension feature is enabled.
// (this prevents you from accidently saving "file.kwd" as RTF, for example)
void KFileWidgetPrivate::updateLocationEditExtension (const QString &lastExtension)
{
    if (!autoSelectExtCheckBox->isChecked() || extension.isEmpty())
        return;

    QString urlStr = locationEditCurrentText();
    if (urlStr.isEmpty())
        return;

    KUrl url = getCompleteUrl(urlStr);
//     kDebug (kfile_area) << "updateLocationEditExtension (" << url << ")";

    const int fileNameOffset = urlStr.lastIndexOf ('/') + 1;
    QString fileName = urlStr.mid (fileNameOffset);

    const int dot = fileName.lastIndexOf ('.');
    const int len = fileName.length();
    if (dot > 0 && // has an extension already and it's not a hidden file
                   // like ".hidden" (but we do accept ".hidden.ext")
        dot != len - 1 // and not deliberately suppressing extension
        )
    {
        // exists?
        KIO::StatJob *statJob = KIO::stat(url, KIO::HideProgressInfo);
        bool result = KIO::NetAccess::synchronousRun(statJob, 0);
        if (result)
        {
//             kDebug (kfile_area) << "\tfile exists";

            if (statJob->statResult().isDir())
            {
//                 kDebug (kfile_area) << "\tisDir - won't alter extension";
                return;
            }

            // --- fall through ---
        }


        //
        // try to get rid of the current extension
        //

        // catch "double extensions" like ".tar.gz"
        if (lastExtension.length() && fileName.endsWith (lastExtension))
            fileName.truncate (len - lastExtension.length());
        else if (extension.length() && fileName.endsWith (extension))
            fileName.truncate (len - extension.length());
        // can only handle "single extensions"
        else
            fileName.truncate (dot);

        // add extension
        const QString newText = urlStr.left (fileNameOffset) + fileName + extension;
        if ( newText != locationEditCurrentText() )
        {
            locationEdit->setItemText(locationEdit->currentIndex(),urlStr.left (fileNameOffset) + fileName + extension);
            locationEdit->lineEdit()->setModified (true);
        }
    }
}

// Updates the filter if the extension of the filename specified in d->locationEdit is changed
// (this prevents you from accidently saving "file.kwd" as RTF, for example)
void KFileWidgetPrivate::updateFilter()
{
//     kDebug(kfile_area);

    if ((operationMode == KFileWidget::Saving) && (ops->mode() & KFile::File) ) {
        const QString urlStr = locationEditCurrentText();
        if (urlStr.isEmpty())
            return;

        KMimeType::Ptr mime = KMimeType::findByPath(urlStr, 0, true);
        if (mime && mime->name() != KMimeType::defaultMimeType()) {
            if (filterWidget->currentFilter() != mime->name() &&
                filterWidget->filters().indexOf(mime->name()) != -1)
                filterWidget->setCurrentFilter(mime->name());
        }
    }
}

// applies only to a file that doesn't already exist
void KFileWidgetPrivate::appendExtension (KUrl &url)
{
//     kDebug(kfile_area);

    if (!autoSelectExtCheckBox->isChecked() || extension.isEmpty())
        return;

    QString fileName = url.fileName();
    if (fileName.isEmpty())
        return;

//     kDebug (kfile_area) << "appendExtension(" << url << ")";

    const int len = fileName.length();
    const int dot = fileName.lastIndexOf ('.');

    const bool suppressExtension = (dot == len - 1);
    const bool unspecifiedExtension = (dot <= 0);

    // don't KIO::Stat if unnecessary
    if (!(suppressExtension || unspecifiedExtension))
        return;

    // exists?
    KIO::StatJob *statJob = KIO::stat(url, KIO::HideProgressInfo);
    bool res = KIO::NetAccess::synchronousRun(statJob, 0);
    if (res)
    {
//         kDebug (kfile_area) << "\tfile exists - won't append extension";
        return;
    }

    // suppress automatically append extension?
    if (suppressExtension)
    {
        //
        // Strip trailing dot
        // This allows lazy people to have autoSelectExtCheckBox->isChecked
        // but don't want a file extension to be appended
        // e.g. "README." will make a file called "README"
        //
        // If you really want a name like "README.", then type "README.."
        // and the trailing dot will be removed (or just stop being lazy and
        // turn off this feature so that you can type "README.")
        //
//         kDebug (kfile_area) << "\tstrip trailing dot";
        url.setFileName (fileName.left (len - 1));
    }
    // evilmatically append extension :) if the user hasn't specified one
    else if (unspecifiedExtension)
    {
//         kDebug (kfile_area) << "\tappending extension \'" << extension << "\'...";
        url.setFileName (fileName + extension);
//         kDebug (kfile_area) << "\tsaving as \'" << url << "\'";
    }
}


// adds the selected files/urls to 'recent documents'
void KFileWidgetPrivate::addToRecentDocuments()
{
    int m = ops->mode();
    int atmost = KRecentDocument::maximumItems();
    //don't add more than we need. KRecentDocument::add() is pretty slow

    if (m & KFile::LocalOnly) {
        const QStringList files = q->selectedFiles();
        QStringList::ConstIterator it = files.begin();
        for ( ; it != files.end() && atmost > 0; ++it ) {
            KRecentDocument::add( *it );
            atmost--;
        }
    }

    else { // urls
        KUrl::List urls = q->selectedUrls();
        KUrl::List::ConstIterator it = urls.begin();
        for ( ; it != urls.end() && atmost > 0; ++it ) {
            if ( (*it).isValid() ) {
                KRecentDocument::add( *it );
                atmost--;
            }
        }
    }
}

KUrlComboBox* KFileWidget::locationEdit() const
{
    return d->locationEdit;
}

KFileFilterCombo* KFileWidget::filterWidget() const
{
    return d->filterWidget;
}

KActionCollection * KFileWidget::actionCollection() const
{
    return d->ops->actionCollection();
}

void KFileWidgetPrivate::_k_toggleSpeedbar(bool show)
{
    if (show) {
        initSpeedbar();
        placesDock->show();
        lafBox->setColumnMinimumWidth(0, placesViewWidth);

        // check to see if they have a home item defined, if not show the home button
        KUrl homeURL;
        homeURL.setPath( QDir::homePath() );
        KFilePlacesModel *model = static_cast<KFilePlacesModel*>(placesView->model());
        for (int rowIndex = 0 ; rowIndex < model->rowCount() ; rowIndex++) {
            QModelIndex index = model->index(rowIndex, 0);
            KUrl url = model->url(index);

            if ( homeURL.equals( url, KUrl::CompareWithoutTrailingSlash ) ) {
                toolbar->removeAction( ops->actionCollection()->action( "home" ) );
                break;
            }
        }
    } else {
        if (q->sender() == placesDock && placesDock && placesDock->isVisibleTo(q)) {
            // we didn't *really* go away! the dialog was simply hidden or
            // we changed virtual desktops or ...
            return;
        }

        if (placesDock) {
            placesDock->hide();
        }

        QAction* homeAction = ops->actionCollection()->action("home");
        QAction* reloadAction = ops->actionCollection()->action("reload");
        if (!toolbar->actions().contains(homeAction)) {
            toolbar->insertAction(reloadAction, homeAction);
        }

        // reset the lafbox to not follow the width of the splitter
        lafBox->setColumnMinimumWidth(0, 0);
    }

    static_cast<KToggleAction *>(q->actionCollection()->action("toggleSpeedbar"))->setChecked(show);
}

void KFileWidgetPrivate::_k_toggleBookmarks(bool show)
{
    if (show)
    {
        if (bookmarkHandler)
        {
            return;
        }

        bookmarkHandler = new KFileBookmarkHandler( q );
        q->connect( bookmarkHandler, SIGNAL( openUrl( const QString& )),
                    SLOT( _k_enterUrl( const QString& )));

        bookmarkButton = new KActionMenu(KIcon("bookmarks"),i18n("Bookmarks"), q);
        bookmarkButton->setDelayed(false);
        q->actionCollection()->addAction("bookmark", bookmarkButton);
        bookmarkButton->setMenu(bookmarkHandler->menu());
        bookmarkButton->setWhatsThis(i18n("<qt>This button allows you to bookmark specific locations. "
                                "Click on this button to open the bookmark menu where you may add, "
                                "edit or select a bookmark.<br /><br />"
                                "These bookmarks are specific to the file dialog, but otherwise operate "
                                "like bookmarks elsewhere in KDE.</qt>"));
        toolbar->addAction(bookmarkButton);
    }
    else if (bookmarkHandler)
    {
        delete bookmarkHandler;
        bookmarkHandler = 0;
        delete bookmarkButton;
        bookmarkButton = 0;
    }

    static_cast<KToggleAction *>(q->actionCollection()->action("toggleBookmarks"))->setChecked( show );
}

// static
KUrl KFileWidget::getStartUrl( const KUrl& startDir,
                               QString& recentDirClass )
{
//     kDebug(kfile_area);

    recentDirClass.clear();
    KUrl ret;

    bool useDefaultStartDir = startDir.isEmpty();
    if ( !useDefaultStartDir )
    {
        if (startDir.protocol() == "kfiledialog")
        {
            if ( startDir.query() == "?global" )
              recentDirClass = QString( "::%1" ).arg( startDir.path().mid( 1 ) );
            else
              recentDirClass = QString( ":%1" ).arg( startDir.path().mid( 1 ) );

            ret = KUrl( KRecentDirs::dir(recentDirClass) );
        }
        else
        {
            ret = startDir;
            // If we won't be able to list it (e.g. http), then use default
            if ( !KProtocolManager::supportsListing( ret ) )
                useDefaultStartDir = true;
        }
    }

    if ( useDefaultStartDir )
    {
        if (lastDirectory->isEmpty()) {
            lastDirectory->setPath(KGlobalSettings::documentPath());
            KUrl home;
            home.setPath( QDir::homePath() );
            // if there is no docpath set (== home dir), we prefer the current
            // directory over it. We also prefer the homedir when our CWD is
            // different from our homedirectory or when the document dir
            // does not exist
            if ( lastDirectory->path(KUrl::AddTrailingSlash) == home.path(KUrl::AddTrailingSlash) ||
                 QDir::currentPath() != QDir::homePath() ||
                 !QDir(lastDirectory->path(KUrl::AddTrailingSlash)).exists() )
                lastDirectory->setPath(QDir::currentPath());
        }
        ret = *lastDirectory;
    }

    return ret;
}

void KFileWidget::setStartDir( const KUrl& directory )
{
    if ( directory.isValid() )
        *lastDirectory = directory;
}

void KFileWidgetPrivate::setNonExtSelection()
{
    // Enhanced rename: Don't highlight the file extension.
    QString filename = locationEditCurrentText();
    QString extension = KMimeType::extractKnownExtension( filename );

    if ( !extension.isEmpty() )
       locationEdit->lineEdit()->setSelection( 0, filename.length() - extension.length() - 1 );
    else
    {
       int lastDot = filename.lastIndexOf( '.' );
       if ( lastDot > 0 )
          locationEdit->lineEdit()->setSelection( 0, lastDot );
    }
}

KToolBar * KFileWidget::toolBar() const
{
    return d->toolbar;
}

void KFileWidget::setCustomWidget(QWidget* widget)
{
    delete d->bottomCustomWidget;
    d->bottomCustomWidget = widget;

    // add it to the dialog, below the filter list box.

    // Change the parent so that this widget is a child of the main widget
    d->bottomCustomWidget->setParent( this );

    d->vbox->addWidget( d->bottomCustomWidget );
    //d->vbox->addSpacing(3); // can't do this every time...

    // FIXME: This should adjust the tab orders so that the custom widget
    // comes after the Cancel button. The code appears to do this, but the result
    // somehow screws up the tab order of the file path combo box. Not a major
    // problem, but ideally the tab order with a custom widget should be
    // the same as the order without one.
    setTabOrder(d->cancelButton, d->bottomCustomWidget);
    setTabOrder(d->bottomCustomWidget, d->urlNavigator);
}

void KFileWidget::setCustomWidget(const QString& text, QWidget* widget)
{
    delete d->labeledCustomWidget;
    d->labeledCustomWidget = widget;

    QLabel* label = new QLabel(text, this);
    label->setAlignment(Qt::AlignRight);
    d->lafBox->addWidget(label, 2, 0, Qt::AlignVCenter);
    d->lafBox->addWidget(widget, 2, 1, Qt::AlignVCenter);
}

void KFileWidget::virtual_hook( int id, void* data )
{
    // this is a workaround to avoid binary compatibility breakage
    // since setConfirmOverwrite in kabstractfilewidget.h is a new function
    // introduced for 4.2. As stated in kabstractfilewidget.h this workaround
    // is going to become a virtual function for KDE5

    switch (id) {
    case 0:
        bool *enable = static_cast<bool*>(data);
        d->confirmOverwrite = *enable;
        break;
    }
}

QString KFileWidgetPrivate::locationEditCurrentText() const
{
    return QDir::fromNativeSeparators(locationEdit->currentText().trimmed());
}

KUrl KFileWidgetPrivate::mostLocalUrl(const KUrl &url)
{
    if (url.isLocalFile()) {
        return url;
    }

    KIO::StatJob *statJob = KIO::stat(url, KIO::HideProgressInfo);
    bool res = KIO::NetAccess::synchronousRun(statJob, 0);

    if (!res) {
        return url;
    }

    const QString path = statJob->statResult().stringValue(KIO::UDSEntry::UDS_LOCAL_PATH);
    if (!path.isEmpty()) {
        KUrl newUrl;
        newUrl.setPath(path);
        return newUrl;
    }

    return url;
}

#include "kfilewidget.moc"
