
#include "example.h"
#include "parts.h"
#include "notepad.h"

#include <QtGui/QSplitter>
#include <QtGui/QCheckBox>
#include <QtCore/QDir>
#include <kcmdlineargs.h>
#include <kiconloader.h>
#include <kstandarddirs.h>
#include <kapplication.h>
#include <kxmlguifactory.h>
#include <kmessagebox.h>
#include <kaction.h>
#include <kactioncollection.h>
#include <klocale.h>
#include <kicon.h>

Shell::Shell()
    : KParts::MainWindow()
{
    // KXMLGUIClient looks in the "data" resource for the .rc files
    // This line is for test programs only!
    KGlobal::dirs()->addResourceDir( "data", KDESRCDIR );

    setXMLFile( "kpartstest_shell.rc" );

    m_manager = new KParts::PartManager( this );

    // When the manager says the active part changes, the builder updates (recreates) the GUI
    connect( m_manager, SIGNAL( activePartChanged( KParts::Part * ) ),
             this, SLOT( createGUI( KParts::Part * ) ) );

    // We can do this "switch active part" because we have a splitter with
    // two items in it.
    // I wonder what kdevelop uses/will use to embed kedit, BTW.
    m_splitter = new QSplitter( this );

    m_part1 = new Part1(this, m_splitter);
    m_part2 = new Part2(this, m_splitter);

    KActionCollection *coll = actionCollection();

    KAction* paOpen = new KAction( "&View local file", this );
    coll->addAction( "open_local_file", paOpen );
    connect(paOpen, SIGNAL(triggered()), this, SLOT(slotFileOpen()));
    KAction* paOpenRemote = new KAction( "&View remote file", this );
    coll->addAction( "open_remote_file", paOpenRemote );
    connect(paOpenRemote, SIGNAL(triggered()), this, SLOT(slotFileOpenRemote()));

    m_paEditFile = new KAction( "&Edit file", this );
    coll->addAction( "edit_file", m_paEditFile );
    connect(m_paEditFile, SIGNAL(triggered()), this, SLOT(slotFileEdit()));
    m_paCloseEditor = new KAction( "&Close file editor", this );
    coll->addAction( "close_editor", m_paCloseEditor );
    connect(m_paCloseEditor, SIGNAL(triggered()), this, SLOT(slotFileCloseEditor()));
    m_paCloseEditor->setEnabled(false);
    KAction * paQuit = new KAction( "&Quit", this );
    coll->addAction( "shell_quit", paQuit );
    connect(paQuit, SIGNAL(triggered()), this, SLOT(close()));
    paQuit->setIcon(KIcon("application-exit"));

//  (void)new KAction( "Yet another menu item", coll, "shell_yami" );
//  (void)new KAction( "Yet another submenu item", coll, "shell_yasmi" );

    KStandardAction::configureToolbars(this, SLOT(configureToolbars() ), actionCollection());
    KStandardAction::keyBindings(guiFactory(), SLOT(configureShortcuts()), actionCollection());

    setCentralWidget( m_splitter );
    m_splitter->setMinimumSize( 400, 300 );

    m_splitter->show();

    m_manager->addPart( m_part1, true ); // sets part 1 as the active part
    m_manager->addPart( m_part2, false );
    m_editorpart = 0;
}

Shell::~Shell()
{
    disconnect( m_manager, 0, this, 0 );
}

void Shell::slotFileOpen()
{
    if ( ! m_part1->openUrl( KStandardDirs::locate("data", KGlobal::mainComponent().componentName()+"/kpartstest_shell.rc" ) ) )
        KMessageBox::error(this,"Couldn't open file !");
}

void Shell::slotFileOpenRemote()
{
    KUrl u ( "http://www.kde.org/index.html" );
    if ( ! m_part1->openUrl( u ) )
        KMessageBox::error(this,"Couldn't open file !");
}

void Shell::embedEditor()
{
    if ( m_manager->activePart() == m_part2 )
        createGUI( 0 );

    // replace part2 with the editor part
    delete m_part2;
    m_part2 = 0;
    m_editorpart = new NotepadPart( m_splitter, this );
    m_editorpart->setReadWrite(); // read-write mode
    m_manager->addPart( m_editorpart );
    m_paEditFile->setEnabled(false);
    m_paCloseEditor->setEnabled(true);
}

void Shell::slotFileCloseEditor()
{
    // It is very important to close the url of a read-write part
    // before destroying it. This allows to save the document (if modified)
    // and also to cancel.
    if ( ! m_editorpart->closeUrl() )
        return;

    // Is this necessary ? (David)
    if ( m_manager->activePart() == m_editorpart )
        createGUI( 0 );

    delete m_editorpart;
    m_editorpart = 0;
    m_part2 = new Part2(this, m_splitter);
    m_manager->addPart( m_part2 );
    m_paEditFile->setEnabled(true);
    m_paCloseEditor->setEnabled(false);
}

void Shell::slotFileEdit()
{
    if ( !m_editorpart )
        embedEditor();
    // TODO use KFileDialog to allow testing remote files
    if ( ! m_editorpart->openUrl( QDir::current().absolutePath()+"/kpartstest_shell.rc" ) )
        KMessageBox::error(this,"Couldn't open file !");
}

int main( int argc, char **argv )
{
    KCmdLineArgs::init( argc, argv, "kpartstest", 0, ki18n("kpartstest"), 0);
    KApplication app;

    Shell *shell = new Shell;

    shell->show();

    app.exec();

    return 0;
}

#include "example.moc"
