 /*
 *   Copyright 2005 by Aaron Seigo <aseigo@kde.org>
 *   Copyright 2007 by Riccardo Iaconelli <riccardo@kde.org>
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

#ifndef PLASMA_APPLET_P_H
#define PLASMA_APPLET_P_H

#include <QGraphicsProxyWidget>

#include <kactioncollection.h>

#include "plasma/animator.h"
#include "plasma/private/dataengineconsumer_p.h"

class KKeySequenceWidget;

namespace Plasma
{

class FrameSvg;
class AppletScript;
class Wallpaper;
class BusyWidget;

class AppletOverlayWidget : public QGraphicsWidget
{
    Q_OBJECT

public:
    AppletOverlayWidget(QGraphicsWidget *parent);
    void destroy();

    qreal opacity;

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);

protected Q_SLOTS:
    void overlayAnimationComplete(QGraphicsItem *item, Plasma::Animator::Animation);
};

class AppletPrivate : public DataEngineConsumer
{
public:
    AppletPrivate(KService::Ptr service, int uniqueID, Applet *applet);
    ~AppletPrivate();

    void init(const QString &packagePath = QString());

    // put all setup routines for script here. at this point we can assume that
    // package exists and that we have a script engin
    void setupScriptSupport();

    /**
     * Sets whether or not this Applet is acting as a Containment
     */
    void setIsContainment(bool isContainment, bool forceUpdate = false);

    QString globalName() const;
    QString instanceName();
    void scheduleConstraintsUpdate(Plasma::Constraints c);
    void scheduleModificationNotification();
    KConfigGroup *mainConfigGroup();
    QString visibleFailureText(const QString &reason);
    void checkImmutability();
    void themeChanged();
    void resetConfigurationObject();
    void appletAnimationComplete(QGraphicsItem *item, Plasma::Animator::Animation anim);
    void selectItemToDestroy();
    void updateRect(const QRectF &rect);
    void setFocus();
    void cleanUpAndDelete();
    void createMessageOverlay(bool usePopup = true);
    void destroyMessageOverlay();
    void addGlobalShortcutsPage(KConfigDialog *dialog);
    void clearShortcutEditorPtr();
    void configDialogFinished();
    KConfigDialog *generateGenericConfigDialog();
    QString configDialogId() const;
    QString configWindowTitle() const;
    void updateShortcuts();

    static KActionCollection* defaultActions(QObject *parent);
    static QSet<QString> knownCategories();

    static uint s_maxAppletId;
    static int s_maxZValue;
    static int s_minZValue;
    static PackageStructure::Ptr packageStructure;
    static QSet<QString> s_customCategories;

    //TODO: examine the usage of memory here; there's a pretty large
    //      number of members at this point.
    uint appletId;
    Applet *q;

    // applet attributes
    Applet::BackgroundHints backgroundHints;
    Applet::BackgroundHints preferredBackgroundHints;
    Plasma::AspectRatioMode aspectRatioMode;
    ImmutabilityType immutability;

    // applet info we keep around in case its needed
    KPluginInfo appletDescription;
    QVariantList args;

    // bookkeeping
    Extender *extender;
    QSet<QGraphicsItem*> registeredAsDragHandle;
    Plasma::FrameSvg *background;
    KConfigGroup *mainConfig;
    Plasma::Constraints pendingConstraints;

    // overlays and messages
    AppletOverlayWidget *messageOverlay;
    QGraphicsProxyWidget *messageOverlayProxy;
    Plasma::BusyWidget *busyWidget;

    // sripting and package stuff
    AppletScript *script;
    Package *package;
    ConfigLoader *configLoader;

    // actions stuff; put activationAction into actions?
    KActionCollection *actions;
    KAction *activationAction;

    // configuration
    KKeySequenceWidget *shortcutEditor; //TODO: subclass KConfigDialog and encapsulate this in there

    ItemStatus itemStatus;

    // timerEvent bookkeeping
    int constraintsTimerId;
    int modificationsTimerId;

    // a great green field of booleans :)
    bool hasConfigurationInterface : 1;
    bool failed : 1;
    bool isContainment : 1;
    bool square : 1;
    bool transient : 1;
    bool needsConfig : 1;
    bool started : 1;
};

} // Plasma namespace

#endif
