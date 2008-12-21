/*
    This file is part of the KDE libraries
    Copyright (C) 2007 Laurent Montel (montel@kde.org)
    Copyright (C) 2007 Christian Ehrlicher (ch.ehrlicher@gmx.de)

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

#include "kwindowsystem.h"

#include <QtGui/QDesktopWidget>
#include <QtGui/QIcon>
#include <QtGui/QBitmap>
#include <QtGui/QPixmap>
#include <QtCore/QLibrary>

#include "kglobal.h"
#include "kdebug.h"
#include "klocalizedstring.h"

#include <windows.h>
#include <windowsx.h>

//functions to register us as taskmanager
typedef bool (WINAPI *PtrSetTaskmanWindow)(HWND);
typedef bool (WINAPI *PtrRegisterShellHookWindow)(HWND);
typedef bool (WINAPI *PtrDeregisterShellHookWindow)(HWND);


#define RSH_UNREGISTER  0
#define RSH_REGISTER    1
#define RSH_TASKMGR     3
typedef bool (WINAPI *PtrRegisterShellHook)(HWND hWnd, DWORD method);


static PtrSetTaskmanWindow pSetTaskmanWindow = 0;
static PtrRegisterShellHookWindow pRegisterShellHookWindow = 0;
static PtrDeregisterShellHookWindow pDeregisterShellHookWindow = 0;
static PtrRegisterShellHook pRegisterShellHook = 0;
static int WM_SHELLHOOK = -1;

class KWindowSystemStaticContainer {
public:
    KWindowSystemStaticContainer() : d(0) {}
    KWindowSystem kwm;
    KWindowSystemPrivate* d;
};

K_GLOBAL_STATIC(KWindowSystemStaticContainer, g_kwmInstanceContainer)

K_GLOBAL_STATIC(QDesktopWidget, s_deskWidget)


struct InternalWindowInfo
{
    InternalWindowInfo(){}
    QPixmap bigIcon;
    QPixmap smallIcon;
    QString windowName;
};

class KWindowSystemPrivate : public QWidget
{
    friend class KWindowSystem;
    public:
        KWindowSystemPrivate ( int what );
        ~KWindowSystemPrivate();  
        
        static bool CALLBACK EnumWindProc (WId hwnd, LPARAM lparam);
        static void readWindowInfo  (WId wid  , InternalWindowInfo *winfo);
        
        void windowAdded        (WId wid);
        void windowRemoved      (WId wid);
        void windowActivated    (WId wid);
        void windowRedraw       (WId wid);
        void windowFlash        (WId wid);
        void windowStateChanged (WId wid);
        void reloadStackList    ( );
        void activate           ( );
        
        
    protected:
        bool winEvent ( MSG * message, long * result );
        
    private:
        int what;
        WId fakeHwnd;
        QList<WId> stackingOrder;
        QMap<WId,InternalWindowInfo> winInfos;
};

static HBITMAP QPixmapMask2HBitmap(const QPixmap &pix)
{
    QBitmap bm = pix.mask();
    if( bm.isNull() ) {
        bm = QBitmap( pix.size() );
        bm.fill( Qt::color1 );
    }
    QImage im = bm.toImage().convertToFormat( QImage::Format_Mono );
    im.invertPixels();                  // funny blank'n'white games on windows
    int w = im.width();
    int h = im.height();
    int bpl = (( w + 15 ) / 16 ) * 2;   // bpl, 16 bit alignment
    QByteArray bits( bpl * h, '\0' );
    for (int y=0; y < h; y++)
        memcpy( bits.data() + y * bpl, im.scanLine( y ), bpl );
    return CreateBitmap( w, h, 1, 1, bits );
}

static HICON QPixmap2HIcon(const QPixmap &pix)
{
    if ( pix.isNull() )
        return 0;

    ICONINFO ii;
    ii.fIcon    = true;
    ii.hbmMask  = QPixmapMask2HBitmap( pix );
    ii.hbmColor = pix.toWinHBITMAP( QPixmap::PremultipliedAlpha );
    ii.xHotspot = 0;
    ii.yHotspot = 0;
    HICON result = CreateIconIndirect( &ii );

    DeleteObject( ii.hbmMask );
    DeleteObject( ii.hbmColor );

    return result;
}

static QPixmap HIcon2QPixmap( HICON hIcon )
{
    ICONINFO ii;
    if( GetIconInfo( hIcon, &ii ) == NULL )
        return QPixmap();

    QPixmap pix  = QPixmap::fromWinHBITMAP( ii.hbmColor );
    pix.setMask( QPixmap::fromWinHBITMAP( ii.hbmMask ) );

    return pix;
}

KWindowSystemPrivate::KWindowSystemPrivate(int what) : QWidget(0)
{
    //i think there is no difference in windows we always load everything
    what = KWindowSystem::INFO_WINDOWS;
    setVisible(false);
}

void KWindowSystemPrivate::activate ( )
{
    //resolve winapi stuff
    if(!pSetTaskmanWindow) pSetTaskmanWindow = (PtrSetTaskmanWindow)QLibrary::resolve("user32","SetTaskmanWindow");
    if(!pRegisterShellHookWindow) pRegisterShellHookWindow = (PtrRegisterShellHookWindow)QLibrary::resolve("user32","RegisterShellHookWindow");
    if(!pDeregisterShellHookWindow) pDeregisterShellHookWindow = (PtrDeregisterShellHookWindow)QLibrary::resolve("user32","DeregisterShellHookWindow");
    if(!pRegisterShellHook) pRegisterShellHook = (PtrRegisterShellHook)QLibrary::resolve("shell32",(LPCSTR)0xb5);
    
    //get the id for the shellhook message
    if(WM_SHELLHOOK==-1) WM_SHELLHOOK  = RegisterWindowMessage(TEXT("SHELLHOOK"));
    
    bool shellHookRegistered = false;
    if(pRegisterShellHook)
        kDebug()<<"use RegisterShellHook";
        shellHookRegistered = pRegisterShellHook(winId(),RSH_REGISTER);
        if(!shellHookRegistered)
            shellHookRegistered = pRegisterShellHook(winId(),RSH_TASKMGR);
    else{     
        kDebug()<<"use RegisterShellHookWindow";
        //i'm not sure if i have to do this, and what happens if some other process uses KWindowSystem
        //if(pSetTaskmanWindow)
        //    pSetTaskmanWindow(winId());
        
        if(pRegisterShellHookWindow)
            shellHookRegistered = pRegisterShellHookWindow(winId());
    }
    
    if(!shellHookRegistered)
        //use a timer and poll the windows ?
         kDebug()<<"Could not create shellhook to receive WindowManager Events";
            
    //fetch window infos
    reloadStackList();
}

KWindowSystemPrivate::~KWindowSystemPrivate()
{
    if(pRegisterShellHook){
        pRegisterShellHook(winId(),RSH_UNREGISTER);
    }else{
        if(pDeregisterShellHookWindow)
            pDeregisterShellHookWindow(winId());
    }
}

/**
 *the callback procedure for the invisible ShellHook window
 */
bool KWindowSystemPrivate::winEvent ( MSG * message, long * result )  
{
    if (message->message == WM_SHELLHOOK) {
        switch(message->wParam) {
          case HSHELL_WINDOWCREATED:
            KWindowSystem::s_d_func()->windowAdded(reinterpret_cast<WId>(message->lParam));
            break;          
          case HSHELL_WINDOWDESTROYED:
            KWindowSystem::s_d_func()->windowRemoved(reinterpret_cast<WId>(message->lParam));
            break;
          case HSHELL_WINDOWACTIVATED:
          case HSHELL_RUDEAPPACTIVATED:
            KWindowSystem::s_d_func()->windowActivated(reinterpret_cast<WId>(message->lParam));
            break;
          case HSHELL_REDRAW: //the caption has changed 
            KWindowSystem::s_d_func()->windowRedraw(reinterpret_cast<WId>(message->lParam));
            break;
          case HSHELL_FLASH:
            KWindowSystem::s_d_func()->windowFlash(reinterpret_cast<WId>(message->lParam));
            break;
          case HSHELL_GETMINRECT:
            KWindowSystem::s_d_func()->windowStateChanged(reinterpret_cast<WId>(message->lParam));
            break;
        }
    }
    return QWidget::winEvent(message,result);
}

bool CALLBACK KWindowSystemPrivate::EnumWindProc(WId hWnd, LPARAM lparam)
{
    QByteArray windowText = QByteArray ( GetWindowTextLength(hWnd)+1, 0 ) ;
    GetWindowTextA(hWnd, windowText.data(), windowText.size());
	DWORD ex_style = GetWindowExStyle(hWnd);
    KWindowSystemPrivate *p = KWindowSystem::s_d_func();
    
    QString add;
    if( !QString(windowText).trimmed().isEmpty() && IsWindowVisible( hWnd ) && !(ex_style&WS_EX_TOOLWINDOW) 
       && !GetParent(hWnd) && !GetWindow(hWnd,GW_OWNER) && !p->winInfos.contains(hWnd) ) {
       
        kDebug()<<"Adding window to windowList " << add + QString(windowText).trimmed();
        
        InternalWindowInfo winfo;
        KWindowSystemPrivate::readWindowInfo(hWnd,&winfo);
        
        p->stackingOrder.append(hWnd);
        p->winInfos.insert(hWnd,winfo);  
    }
    return true; 
}

void KWindowSystemPrivate::readWindowInfo ( WId hWnd , InternalWindowInfo *winfo)
{
    QByteArray windowText = QByteArray ( GetWindowTextLength(hWnd)+1, 0 ) ;
    GetWindowTextA(hWnd, windowText.data(), windowText.size());
     //maybe use SendMessageTimout here?
    QPixmap smallIcon;
    HICON hSmallIcon = (HICON)SendMessage(hWnd, WM_GETICON, ICON_SMALL, 0);
    //if(!hSmallIcon) hSmallIcon = (HICON)SendMessage(hWnd, WM_GETICON, ICON_SMALL2, 0);
    if(!hSmallIcon) hSmallIcon = (HICON)SendMessage(hWnd, WM_GETICON, ICON_BIG, 0);
    if(!hSmallIcon) hSmallIcon = (HICON)GetClassLong(hWnd, GCL_HICONSM);
    if(!hSmallIcon) hSmallIcon = (HICON)GetClassLong(hWnd, GCL_HICON);
    if(!hSmallIcon) hSmallIcon = (HICON)SendMessage(hWnd, WM_QUERYDRAGICON, 0, 0);
    if(hSmallIcon)  smallIcon  = HIcon2QPixmap(hSmallIcon);
    
    QPixmap bigIcon;
    HICON hBigIcon = (HICON)SendMessage(hWnd, WM_GETICON, ICON_BIG, 0);
    //if(!hBigIcon) hBigIcon = (HICON)SendMessage(hWnd, WM_GETICON, ICON_SMALL2, 0);
    if(!hBigIcon) hBigIcon = (HICON)SendMessage(hWnd, WM_GETICON, ICON_SMALL, 0);
    if(!hBigIcon) hBigIcon = (HICON)GetClassLong(hWnd, GCL_HICON);
    if(!hBigIcon) hBigIcon = (HICON)GetClassLong(hWnd, GCL_HICONSM);
    if(!hBigIcon) hBigIcon = (HICON)SendMessage(hWnd, WM_QUERYDRAGICON, 0, 0);
    if(hBigIcon)  bigIcon  = HIcon2QPixmap(hBigIcon);
       
    winfo->bigIcon    = bigIcon;
    winfo->smallIcon  = smallIcon;
    winfo->windowName = QString(windowText).trimmed();   
}


void KWindowSystemPrivate::windowAdded     (WId wid)
{
    KWindowSystem::s_d_func()->reloadStackList();
    emit KWindowSystem::self()->windowAdded(wid);
    emit KWindowSystem::self()->stackingOrderChanged();
}

void KWindowSystemPrivate::windowRemoved   (WId wid)
{
    KWindowSystem::s_d_func()->reloadStackList();
    emit KWindowSystem::self()->windowRemoved(wid);
    emit KWindowSystem::self()->stackingOrderChanged();
}

void KWindowSystemPrivate::windowActivated (WId wid)
{
    KWindowSystem::s_d_func()->reloadStackList();
    emit KWindowSystem::self()->activeWindowChanged(wid);
    emit KWindowSystem::self()->stackingOrderChanged();
}

void KWindowSystemPrivate::windowRedraw    (WId wid)
{
    KWindowSystem::s_d_func()->reloadStackList();
}

void KWindowSystemPrivate::windowFlash     (WId wid)
{
    //emit KWindowSystem::self()->demandAttention( wid );
}

void KWindowSystemPrivate::windowStateChanged (WId wid)
{
    emit KWindowSystem::self()->windowChanged( wid );
}

void KWindowSystemPrivate::reloadStackList ()
{
    KWindowSystem::s_d_func()->stackingOrder.clear();
    KWindowSystem::s_d_func()->winInfos.clear();
    EnumWindows((WNDENUMPROC)EnumWindProc, 0 );
}



KWindowSystem* KWindowSystem::self()
{
    return &(g_kwmInstanceContainer->kwm);
}

KWindowSystemPrivate* KWindowSystem::s_d_func()
{
    return g_kwmInstanceContainer->d;
}

void KWindowSystem::init(int what)
{
    KWindowSystemPrivate* const s_d = s_d_func();

    if (what >= INFO_WINDOWS)
       what = INFO_WINDOWS;
    else
       what = INFO_BASIC;

    if ( !s_d )
    {
        g_kwmInstanceContainer->d = new KWindowSystemPrivate(what); // invalidates s_d
        g_kwmInstanceContainer->d->activate();
    }
    else if (s_d->what < what)
    {
        delete s_d;
        g_kwmInstanceContainer->d = new KWindowSystemPrivate(what); // invalidates s_d
        g_kwmInstanceContainer->d->activate();
    }

}

int KWindowSystem::currentDesktop()
{
    return 1;
}

int KWindowSystem::numberOfDesktops()
{
    return 1;
}

void KWindowSystem::setMainWindow( QWidget* subwindow, WId mainwindow )
{
    KWindowSystem::init(INFO_WINDOWS);
    SetForegroundWindow(subwindow->winId());
}

void KWindowSystem::setCurrentDesktop( int desktop )
{
    KWindowSystem::init(INFO_WINDOWS);
    kDebug() << "KWindowSystem::setCurrentDesktop( int desktop ) isn't yet implemented!";
    //TODO
}

void KWindowSystem::setOnAllDesktops( WId win, bool b )
{
    KWindowSystem::init(INFO_WINDOWS);
     kDebug() << "KWindowSystem::setOnAllDesktops( WId win, bool b ) isn't yet implemented!";
     //TODO
}

void KWindowSystem::setOnDesktop( WId win, int desktop )
{
    KWindowSystem::init(INFO_WINDOWS);
     //TODO
     kDebug() << "KWindowSystem::setOnDesktop( WId win, int desktop ) isn't yet implemented!";
}

WId KWindowSystem::activeWindow()
{
    KWindowSystem::init(INFO_WINDOWS);
    return GetActiveWindow();
}

void KWindowSystem::activateWindow( WId win, long )
{
    KWindowSystem::init(INFO_WINDOWS);
    SetActiveWindow( win );
}

void KWindowSystem::forceActiveWindow( WId win, long time )
{
    KWindowSystem::init(INFO_WINDOWS);
    SetActiveWindow( win );
    SetForegroundWindow( win );
}

void KWindowSystem::demandAttention( WId win, bool set )
{
    KWindowSystem::init(INFO_WINDOWS);
    FLASHWINFO fi;
    fi.cbSize = sizeof( FLASHWINFO );
    fi.hwnd = win;
    fi.dwFlags = set ? FLASHW_ALL : FLASHW_STOP;
    fi.uCount = 5;
    fi.dwTimeout = 0;

    FlashWindowEx( &fi );
}


QPixmap KWindowSystem::icon( WId win, int width, int height, bool scale )
{
    KWindowSystem::init(INFO_WINDOWS);
    
    QPixmap pm;
    if(KWindowSystem::s_d_func()->winInfos.contains(win)){
        if( width < 24 || height < 24 )
            pm = KWindowSystem::s_d_func()->winInfos[win].smallIcon;
        else
            pm = KWindowSystem::s_d_func()->winInfos[win].bigIcon;
    }
    else{
        kDebug()<<"KWindowSystem::icon winid not in winInfos";
        UINT size = ICON_BIG;
        if( width < 24 || height < 24 )
            size = ICON_SMALL;
        HICON hIcon = (HICON)SendMessage( win, WM_GETICON, size, 0);
        pm = HIcon2QPixmap( hIcon );
    } 
    if( scale )
        pm = pm.scaled( width, height );
    return pm;
}

QPixmap KWindowSystem::icon( WId win, int width, int height, bool scale, int )
{
    return icon( win, width, height, scale );
}

void KWindowSystem::setIcons( WId win, const QPixmap& icon, const QPixmap& miniIcon )
{
    KWindowSystem::init(INFO_WINDOWS);
    KWindowSystemPrivate* s_d = s_d_func();
    
    if(s_d->winInfos.contains(win)){
        // is this safe enough or do i have to refresh() the window infos
        s_d->winInfos[win].smallIcon = miniIcon;
        s_d->winInfos[win].bigIcon   = icon;
    }
    
    HICON hIconBig = QPixmap2HIcon(icon);
    HICON hIconSmall = QPixmap2HIcon(miniIcon);

    hIconBig = (HICON)SendMessage( win, WM_SETICON, ICON_BIG,   (LPARAM)hIconBig );
    hIconSmall = (HICON)SendMessage( win, WM_SETICON, ICON_SMALL, (LPARAM)hIconSmall );
    
}

void KWindowSystem::setState( WId win, unsigned long state )
{
    KWindowSystem::init(INFO_WINDOWS);
    bool got = false;
    if (state & NET::SkipTaskbar) {
        got = true;
        LONG_PTR lp = GetWindowLongPtr(win, GWL_EXSTYLE);
        SetWindowLongPtr(win, GWL_EXSTYLE, lp | WS_EX_TOOLWINDOW);
    }
    if (state & NET::KeepAbove) {
        got = true;
        SetWindowPos(win, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    }
    if(state & NET::KeepBelow){
        got = true;
        SetWindowPos(win, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    }
    if (!got)
        kDebug() << "KWindowSystem::setState( WId win, unsigned long state ) isn't yet implemented for the state you requested!";
}

void KWindowSystem::clearState( WId win, unsigned long state )
{
    KWindowSystem::init(INFO_WINDOWS);
    //TODO
    kDebug() << "KWindowSystem::clearState( WId win, unsigned long state ) isn't yet implemented!";
}

void KWindowSystem::minimizeWindow( WId win, bool animation)
{
    Q_UNUSED( animation );
    KWindowSystem::init(INFO_WINDOWS);    
    ShowWindow( win, SW_MINIMIZE );
}

void KWindowSystem::unminimizeWindow( WId win, bool animation )
{
    Q_UNUSED( animation );
    KWindowSystem::init(INFO_WINDOWS);    
    ShowWindow( win, SW_RESTORE );
}

void KWindowSystem::raiseWindow( WId win )
{
    KWindowSystem::init(INFO_WINDOWS);
    SetWindowPos( win, HWND_TOP, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE ); // mhhh?
}

void KWindowSystem::lowerWindow( WId win )
{
    KWindowSystem::init(INFO_WINDOWS);
    SetWindowPos( win, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE ); // mhhh?
}

bool KWindowSystem::compositingActive()
{
    return false;
}

QRect KWindowSystem::workArea( int desktop )
{
    KWindowSystem::init(INFO_WINDOWS);
    return s_deskWidget->availableGeometry( desktop );
}

QRect KWindowSystem::workArea( const QList<WId>& exclude, int desktop )
{
    //TODO
    kDebug() << "QRect KWindowSystem::workArea( const QList<WId>& exclude, int desktop ) isn't yet implemented!";
    KWindowSystem::init(INFO_WINDOWS);    
    return QRect();
}

QString KWindowSystem::desktopName( int desktop )
{
    return i18n("Desktop %1",  desktop );
}

void KWindowSystem::setDesktopName( int desktop, const QString& name )
{
     kDebug() << "KWindowSystem::setDesktopName( int desktop, const QString& name ) isn't yet implemented!";  
    //TODO
}

bool KWindowSystem::showingDesktop()
{
    return false;
}

void KWindowSystem::setUserTime( WId win, long time )
{
    kDebug() << "KWindowSystem::setUserTime( WId win, long time ) isn't yet implemented!";
    //TODO
}

bool KWindowSystem::icccmCompliantMappingState()
{
    return false;
}

// optimalization - create KWindowSystemPrivate only when needed and only for what is needed
void KWindowSystem::connectNotify( const char* signal )
{
    int what = INFO_BASIC;
    if( QLatin1String( signal ) == SIGNAL(workAreaChanged()))
        what = INFO_WINDOWS;
    else if( QLatin1String( signal ) == SIGNAL(strutChanged()))
        what = INFO_WINDOWS;
    else if( QLatin1String( signal ) == QMetaObject::normalizedSignature(SIGNAL(windowChanged(WId,const unsigned long*))).constData())
        what = INFO_WINDOWS;
    else if( QLatin1String( signal ) ==  QMetaObject::normalizedSignature(SIGNAL(windowChanged(WId,unsigned int))).constData())
        what = INFO_WINDOWS;
    else if( QLatin1String( signal ) ==  QMetaObject::normalizedSignature(SIGNAL(windowChanged(WId))).constData())
        what = INFO_WINDOWS;

    init( what );
    QObject::connectNotify( signal );
}

void KWindowSystem::setExtendedStrut( WId win, int left_width, int left_start, int left_end,
        int right_width, int right_start, int right_end, int top_width, int top_start, int top_end,
        int bottom_width, int bottom_start, int bottom_end )
{
  kDebug() << "KWindowSystem::setExtendedStrut isn't yet implemented!";
  //TODO
}
void KWindowSystem::setStrut( WId win, int left, int right, int top, int bottom )
{
  kDebug() << "KWindowSystem::setStrut isn't yet implemented!";
  //TODO
}

QString KWindowSystem::readNameProperty( WId window, unsigned long atom )
{
  //TODO
  kDebug() << "QString KWindowSystem::readNameProperty( WId window, unsigned long atom ) isn't yet implemented!";
  return QString();
}

void KWindowSystem::doNotManage( const QString& title )
{
  //TODO
  kDebug() << "KWindowSystem::doNotManage( const QString& title ) isn't yet implemented!";
}

QList<WId> KWindowSystem::stackingOrder()
{
  KWindowSystem::init(INFO_WINDOWS);  
  return KWindowSystem::s_d_func()->stackingOrder;
}

const QList<WId>& KWindowSystem::windows()
{
  KWindowSystem::init(INFO_WINDOWS);  
  return KWindowSystem::s_d_func()->stackingOrder;
}

void KWindowSystem::setType( WId win, NET::WindowType windowType )
{
 //TODO
 KWindowSystem::init(INFO_WINDOWS); 
 kDebug() << "setType( WId win, NET::WindowType windowType ) isn't yet implemented!";
}

KWindowInfo KWindowSystem::windowInfo( WId win, unsigned long properties, unsigned long properties2 )
{
    KWindowSystem::init(INFO_WINDOWS);
    return KWindowInfo( win, properties, properties2 );
}

bool KWindowSystem::hasWId(WId w)
{
    KWindowSystem::init(INFO_WINDOWS);
    return KWindowSystem::s_d_func()->winInfos.contains(w);
}

#include "kwindowsystem.moc"
