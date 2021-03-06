/*
 *   Copyright 2007 by Aaron Seigo <aseigo@kde.org>
 *   Copyright 2008 by Ménard Alexis <darktears31@gmail.com>
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

#ifndef CONTAINMENT_P_H
#define CONTAINMENT_P_H

#include <kmenu.h>

static const int INTER_CONTAINMENT_MARGIN = 6;
static const int TOOLBOX_MARGIN = 150;
static const int CONTAINMENT_COLUMNS = 2;
static const int VERTICAL_STACKING_OFFSET = 10000;

namespace Plasma
{

class Containment;
class ToolBox;

class ContainmentPrivate
{
public:
    ContainmentPrivate(Containment *c)
        : q(c),
          formFactor(Planar),
          location(Floating),
          focusedApplet(0),
          wallpaper(0),
          screen(-1), // no screen
          desktop(-1), // all desktops
          toolBox(0),
          con(0),
          type(Containment::NoContainmentType),
          drawWallpaper(true)
    {
    }

    ~ContainmentPrivate()
    {
        qDeleteAll(applets);
        applets.clear();
    }

    ToolBox *createToolBox();
    void positionToolBox();
    void updateToolBoxVisibility();
    void triggerShowAddWidgets();
    void requestConfiguration();

    /**
     * Called when constraints have been updated on this containment to provide
     * constraint services common to all containments. Containments should still
     * implement their own constraintsEvent method
     */
    void containmentConstraintsEvent(Plasma::Constraints constraints);

    bool regionIsEmpty(const QRectF &region, Applet *ignoredApplet=0) const;
    void positionPanel(bool force = false);
    void positionContainments();
    void setLockToolText();
    void handleDisappeared(AppletHandle *handle);
    void appletDestroyed(Plasma::Applet*);
    void containmentAppletAnimationComplete(QGraphicsItem *item, Plasma::Animator::Animation anim);
    void zoomIn();
    void zoomOut();
    void mimeTypeRetrieved(KIO::Job *job, const QString &mimetype);
    void containmentActions(KMenu &desktopMenu);
    void appletActions(KMenu &desktopMenu, Applet *applet, bool includeApplet);
    bool showContextMenu(const QPointF &point, const QPoint &screenPos, bool includeApplet);
    void checkRemoveAction();

    Applet *addApplet(const QString &name, const QVariantList &args = QVariantList(),
                      const QRectF &geometry = QRectF(-1, -1, -1, -1), uint id = 0,
                      bool delayedInit = false);

    KActionCollection *actions();

    /**
     * add the regular actions & keyboard shortcuts onto Applet's collection
     */
    static void addDefaultActions(KActionCollection *actions);

    /**
     * give keyboard focus to applet within this containment
     */
    void focusApplet(Plasma::Applet *applet);

    /**
     * returns the Context for this Containment
     */
    Context *context();

    /**
     * Handles dropped/pasted mimetype data
     * @param event the drop or mouse event
     */
    void dropData(QGraphicsSceneEvent *event);

    Containment *q;
    FormFactor formFactor;
    Location location;
    Applet::List applets;
    Applet *focusedApplet;
    Plasma::Wallpaper *wallpaper;
    QMap<Applet*, AppletHandle*> handles;
    int screen;
    int desktop;
    ToolBox *toolBox;
    Context *con;
    Containment::Type type;
    static bool s_positioning;
    bool drawWallpaper;
    QHash<KIO::Job*, QPointF> dropPoints;
    QHash<KIO::Job*, QMenu*> dropMenus;

};

} // Plasma namespace

#endif
