/*
 *   Copyright 2007 Aaron Seigo <aseigo@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "view.h"

#include <kglobal.h>
#include <kwindowsystem.h>
#include <kactioncollection.h>

#include "corona.h"
#include "containment.h"
#include "wallpaper.h"

using namespace Plasma;

namespace Plasma
{

class ViewPrivate
{
public:
    ViewPrivate(View *view, int uniqueId)
        : q(view),
          containment(0),
          drawWallpaper(true),
          trackChanges(true),
          viewId(0)
    {
        if (uniqueId > s_maxViewId) {
            s_maxViewId = uniqueId;
            viewId = uniqueId;
        }

        if (viewId == 0) {
            // we didn't get a sane value assigned to us, so lets
            // grab the next available id
            viewId = ++s_maxViewId;
        }
    }

    ~ViewPrivate()
    {
    }

    void updateSceneRect()
    {
        if (!containment || !trackChanges) {
            return;
        }

        kDebug() << "!!!!!!!!!!!!!!!!! setting the scene rect to"
                 << containment->sceneBoundingRect()
                 << "associated screen is" << containment->screen();

        emit q->sceneRectAboutToChange();
        if (q->transform().isIdentity()) { //we're not zoomed out
            q->setSceneRect(containment->sceneBoundingRect());
        } else {
            //kDebug() << "trying to show the containment nicely";
            q->ensureVisible(containment->sceneBoundingRect());
            //q->centerOn(containment);
        }
        emit q->sceneRectChanged();
    }

    void containmentDestroyed()
    {
        containment = 0;
    }

    void initGraphicsView()
    {
        q->setFrameShape(QFrame::NoFrame);
        q->setAutoFillBackground(true);
        q->setDragMode(QGraphicsView::NoDrag);
        //setCacheMode(QGraphicsView::CacheBackground);
        q->setInteractive(true);
        q->setAcceptDrops(true);
        q->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        q->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        q->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    }

    Plasma::View *q;
    Plasma::Containment *containment;
    bool drawWallpaper;
    bool trackChanges;
    int viewId;
    static int s_maxViewId;
};

int ViewPrivate::s_maxViewId(0);

View::View(Containment *containment, QWidget *parent)
    : QGraphicsView(parent),
      d(new ViewPrivate(this, 0))
{
    d->initGraphicsView();

    if (containment) {
        setScene(containment->scene());
        setContainment(containment);
    }
}

View::View(Containment *containment, int viewId, QWidget *parent)
    : QGraphicsView(parent),
      d(new ViewPrivate(this, viewId))
{
    d->initGraphicsView();

    if (containment) {
        setScene(containment->scene());
        setContainment(containment);
    }
}

View::~View()
{
    delete d;
    // FIXME FIX a focus crash but i wasn't able to reproduce in a simple test case for Qt guys
    //       NB: this is also done in Corona
    clearFocus();
}

void View::setScreen(int screen, int desktop)
{
    if (screen > -1) {
        Corona *corona = qobject_cast<Corona*>(scene());

        if (!corona) {
            return;
        }

        // -1 == All desktops
        if (desktop < -1 || desktop > KWindowSystem::numberOfDesktops() - 1) {
            desktop = -1;
        }

        Containment *containment = corona->containmentForScreen(screen, desktop);
        if (containment) {
            d->containment = 0; //so that we don't end up on the old containment's screen
            setContainment(containment);
        }
    }
}

int View::screen() const
{
    if (d->containment) {
        return d->containment->screen();
    }

    return -1;
}

int View::desktop() const
{
    if (d->containment) {
        return d->containment->desktop();
    }

    return -2;
}

int View::effectiveDesktop() const
{
    int desk = desktop();
    return desk > -1 ? desk : KWindowSystem::currentDesktop();
}

void View::setContainment(Plasma::Containment *containment)
{
    if (containment == d->containment) {
        return;
    }

    if (d->containment) {
        disconnect(containment, SIGNAL(destroyed(QObject*)), this, SLOT(containmentDestroyed()));
        disconnect(d->containment, SIGNAL(geometryChanged()), this, SLOT(updateSceneRect()));
        d->containment->removeAssociatedWidget(this);
    }

    if (!containment) {
        d->containment = 0;
        return;
    }

    Containment *oldContainment = d->containment;

    int screen = -1;
    int desktop = -1;
    if (oldContainment) {
        screen = d->containment->screen();
        desktop = d->containment->desktop();
    } else {
        setScene(containment->scene());
    }

    d->containment = containment;

    //add keyboard-shortcut actions
    d->containment->addAssociatedWidget(this);

    int otherScreen = containment->screen();
    int otherDesktop = containment->desktop();

    if (screen > -1) {
        containment->setScreen(screen, desktop);
    }

    if (oldContainment && otherScreen > -1) {
        // assign the old containment the old screen/desktop
        oldContainment->setScreen(otherScreen, otherDesktop);
    }

    /*
    if (oldContainment) {
        kDebug() << (QObject*)oldContainment << screen << oldContainment->screen()
                 << (QObject*)containment << otherScreen << containment->screen();
    }
    */

    d->updateSceneRect();
    connect(containment, SIGNAL(destroyed(QObject*)), this, SLOT(containmentDestroyed()));
    connect(containment, SIGNAL(geometryChanged()), this, SLOT(updateSceneRect()));
}

Containment *View::containment() const
{
    return d->containment;
}

Containment *View::swapContainment(const QString &name, const QVariantList &args)
{
    return swapContainment(d->containment, name, args);
}

Containment *View::swapContainment(Plasma::Containment *existing, const QString &name, const QVariantList &args)
{
    if (!existing) {
        return 0;
    }

    Containment *old = existing;
    Plasma::Corona *corona = old->corona();
    Plasma::Containment *c = corona->addContainment(name, args);
    if (c) {
        KConfigGroup oldConfig = old->config();
        KConfigGroup newConfig = c->config();

        // ensure that the old containments configuration is up to date
        old->save(oldConfig);

        // Copy configuration to new containment
        oldConfig.copyTo(&newConfig);

        if (old == d->containment) {
            // set our containment to the new one, if the the old containment was us
            setContainment(c);
        }

        // load the configuration of the old containment into the new one
        c->restore(newConfig);
        foreach (Applet *applet, c->applets()) {
            applet->init();
            // We have to flush the applet constraints manually
            applet->flushPendingConstraintsEvents();
        }

        // destroy the old one
        old->destroy(false);

        // and now save the config
        c->save(newConfig);
        corona->requestConfigSync();

        return c;
    }

    return old;
}

KConfigGroup View::config() const
{
    KConfigGroup views(KGlobal::config(), "PlasmaViews");
    return KConfigGroup(&views, QString::number(d->viewId));
}

int View::id() const
{
    return d->viewId;
}

void View::setWallpaperEnabled(bool draw)
{
    d->drawWallpaper = draw;
}

bool View::isWallpaperEnabled() const
{
    return d->drawWallpaper;
}

void View::setTrackContainmentChanges(bool trackChanges)
{
    d->trackChanges = trackChanges;
}

bool View::trackContainmentChanges()
{
    return d->trackChanges;
}

View * View::topLevelViewAt(const QPoint & pos)
{
    QWidget *w = QApplication::topLevelAt(pos);
    if (w) {
        Plasma::View *v = qobject_cast<Plasma::View *>(w);
        return v;
    } else {
        return 0;
    }
}

} // namespace Plasma

#include "view.moc"

