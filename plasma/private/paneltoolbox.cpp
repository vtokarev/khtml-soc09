/*
 *   Copyright 2007 by Aaron Seigo <aseigo@kde.org>
 *   Copyright 2008 by Marco Martin <notmart@gmail.com>
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

#include "paneltoolbox_p.h"

#include <QGraphicsSceneHoverEvent>
#include <QPainter>
#include <QRadialGradient>
#include <QApplication>

#include <kcolorscheme.h>
#include <kdebug.h>

#include <plasma/applet.h>
#include <plasma/paintutils.h>
#include <plasma/theme.h>
#include <plasma/svg.h>

namespace Plasma
{

class PanelToolBoxPrivate
{
public:
    PanelToolBoxPrivate()
      : icon("plasma"),
        animId(0),
        animFrame(0),
        highlighting(false)
    {
    }


    KIcon icon;
    int animId;
    qreal animFrame;
    QColor fgColor;
    QColor bgColor;
    Plasma::Svg *background;
    bool highlighting;
};

PanelToolBox::PanelToolBox(Containment *parent)
    : ToolBox(parent),
      d(new PanelToolBoxPrivate)
{
    connect(this, SIGNAL(toggled()), this, SLOT(toggle()));

    setZValue(10000000);
    setFlag(ItemClipsChildrenToShape, false);
    //panel toolbox is allowed to zoom, otherwise a part of it will be displayed behind the desktop
    //toolbox when the desktop is zoomed out
    setFlag(ItemIgnoresTransformations, false);
    assignColors();
    connect(Plasma::Theme::defaultTheme(), SIGNAL(themeChanged()),
            this, SLOT(assignColors()));

    d->background = new Plasma::Svg(this);
    d->background->setImagePath("widgets/toolbox");
    d->background->setContainsMultipleImages(true);
}

PanelToolBox::~PanelToolBox()
{
    delete d;
}

void PanelToolBox::assignColors()
{
    d->bgColor = Plasma::Theme::defaultTheme()->color(Plasma::Theme::BackgroundColor);
    d->fgColor = Plasma::Theme::defaultTheme()->color(Plasma::Theme::TextColor);
}

QRectF PanelToolBox::boundingRect() const
{
    QRectF r;

    //Only Left,Right and Bottom supported, default to Right
    if (corner() == ToolBox::Bottom) {
        r = QRectF(0, 0, size() * 2, size());
    } else if (corner() == ToolBox::Left) {
        r = QRectF(0, 0, size(), size() * 2);
    } else {
        r = QRectF(0, 0, size(), size() * 2);
    }

    if (parentItem()) {
        QSizeF s = parentItem()->boundingRect().size();

        if (r.height() > s.height()) {
            r.setHeight(s.height());
        }

        if (r.width() > s.width()) {
            r.setWidth(s.width());
        }
    }

    return r;
}

void PanelToolBox::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)

    const qreal progress = d->animFrame / size();

    QRect backgroundRect;
    QPoint gradientCenter;
    QRectF rect = boundingRect();
    QString cornerElement;

    if (corner() == ToolBox::Bottom) {
        gradientCenter = QPoint(rect.center().x(), rect.bottom());
        cornerElement = "panel-south";

        backgroundRect = d->background->elementRect(cornerElement).toRect();
        backgroundRect.moveBottomLeft(shape().boundingRect().bottomLeft().toPoint());
    } else if (corner() == ToolBox::Right) {
        gradientCenter = QPoint(rect.right(), rect.center().y());
        cornerElement = "panel-east";

        backgroundRect = d->background->elementRect(cornerElement).toRect();
        backgroundRect.moveTopRight(shape().boundingRect().topRight().toPoint());
    } else {
        gradientCenter = QPoint(rect.right(), rect.center().y());
        cornerElement = "panel-west";

        backgroundRect = d->background->elementRect(cornerElement).toRect();
        backgroundRect.moveTopLeft(shape().boundingRect().topLeft().toPoint());
    }


    d->background->paint(painter, backgroundRect, cornerElement);


    QRect iconRect;

    //Only Left,Right and Bottom supported, default to Right
    if (corner() == ToolBox::Bottom) {
        iconRect = QRect(QPoint(gradientCenter.x() - iconSize().width() / 2,
                                (int)rect.bottom() - iconSize().height() - 2), iconSize());
    } else if (corner() == ToolBox::Left) {
        iconRect = QRect(QPoint(2, gradientCenter.y() - iconSize().height() / 2), iconSize());
    } else {
        iconRect = QRect(QPoint((int)rect.right() - iconSize().width() + 1,
                                gradientCenter.y() - iconSize().height() / 2), iconSize());
    }

    if (qFuzzyCompare(qreal(1.0), progress)) {
        d->icon.paint(painter, iconRect);
    } else if (qFuzzyCompare(qreal(1.0), 1 + progress)) {
        d->icon.paint(painter, iconRect, Qt::AlignCenter, QIcon::Disabled, QIcon::Off);
    } else {
        QPixmap disabled = d->icon.pixmap(iconSize(), QIcon::Disabled, QIcon::Off);
        QPixmap enabled = d->icon.pixmap(iconSize());
        QPixmap result = PaintUtils::transition(
            d->icon.pixmap(iconSize(), QIcon::Disabled, QIcon::Off),
            d->icon.pixmap(iconSize()), progress);
        painter->drawPixmap(iconRect, result);
    }
}

QPainterPath PanelToolBox::shape() const
{
    QPainterPath path;
    int toolSize = size();// + (int)d->animFrame;
    QRectF rect = boundingRect();

    //Only Left,Right and Bottom supported, default to Right
    if (corner() == ToolBox::Bottom) {
        path.moveTo(rect.bottomLeft());
        path.arcTo(QRectF(rect.center().x() - toolSize,
                          rect.bottom() - toolSize,
                          toolSize * 2,
                          toolSize * 2), 0, 180);
    } else if (corner() == ToolBox::Left) {
        path.arcTo(QRectF(rect.left(),
                          rect.center().y() - toolSize,
                          toolSize * 2,
                          toolSize * 2), 90, -180);
    } else {
        path.moveTo(rect.topRight());
        path.arcTo(QRectF(rect.left(),
                          rect.center().y() - toolSize,
                          toolSize * 2,
                          toolSize * 2), 90, 180);
    }

    return path;
}

void PanelToolBox::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    highlight(true);
    QGraphicsItem::hoverEnterEvent(event);
}

void PanelToolBox::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    //kDebug() << event->pos() << event->scenePos()
    if (!showing()) {
        highlight(false);
    }

    QGraphicsItem::hoverLeaveEvent(event);
}

void PanelToolBox::showToolBox()
{
    setShowing(true);
    highlight(true);
}

void PanelToolBox::hideToolBox()
{
    setShowing(false);
    highlight(false);
}

void PanelToolBox::highlight(bool highlighting)
{
    if (d->highlighting == highlighting) {
        return;
    }

    Plasma::Animator *animdriver = Plasma::Animator::self();

    if (d->animId) {
        animdriver->stopCustomAnimation(d->animId);
    }

    d->highlighting = highlighting;
    d->animId = animdriver->customAnimation(10, 240, 
                                            highlighting ? Plasma::Animator::EaseInCurve 
                                                         : Plasma::Animator::EaseOutCurve,
                                            this, "animate");
}

void PanelToolBox::animate(qreal progress)
{
    if (d->highlighting) {
        d->animFrame = size() * progress;
    } else {
        d->animFrame = size() * (1.0 - progress);
    }

    //kDebug() << "animating at" << progress << "for" << d->animFrame;

    if (progress >= 1) {
        d->animId = 0;
    }

    update();
}

void PanelToolBox::toggle()
{
    setShowing(!showing());
    highlight(showing());
}

} // plasma namespace

#include "paneltoolbox_p.moc"

