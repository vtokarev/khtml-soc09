/*
 *   Copyright 2006-2007 Aaron Seigo <aseigo@kde.org>
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

#include "svg.h"

#include <QDir>
#include <QMatrix>
#include <QPainter>
#include <QSharedData>
#include <QTimer>

#include <kcolorscheme.h>
#include <kconfiggroup.h>
#include <kdebug.h>
#include <kiconeffect.h>
#include <kglobalsettings.h>
#include <ksharedptr.h>
#include <ksvgrenderer.h>

#include "applet.h"
#include "package.h"
#include "theme.h"

namespace Plasma
{

class SharedSvgRenderer : public KSvgRenderer, public QSharedData
{
    public:
        typedef KSharedPtr<SharedSvgRenderer> Ptr;

        SharedSvgRenderer(QObject *parent = 0)
            : KSvgRenderer(parent)
        {}

        SharedSvgRenderer(const QString &filename, QObject *parent = 0)
            : KSvgRenderer(filename, parent)
        {}

        SharedSvgRenderer(const QByteArray &contents, QObject *parent = 0)
            : KSvgRenderer(contents, parent)
        {}

        ~SharedSvgRenderer()
        {
            //kDebug() << "leaving this world for a better one.";
        }
};

class SvgPrivate
{
    public:
        SvgPrivate(Svg *svg)
            : q(svg),
              saveTimer(0),
              renderer(0),
              multipleImages(false),
              themed(false),
              applyColors(false)
        {
        }

        ~SvgPrivate()
        {
            eraseRenderer();
        }

        //This function is meant for the rects cache
        QString cacheId(const QString &elementId)
        {
            if (size.isValid() && size != naturalSize) {
                return QString("%3_%2_%1").arg(int(size.height()))
                                        .arg(int(size.width()))
                                        .arg(elementId);
            } else {
                return QString("%2_%1").arg("Natural")
                                        .arg(elementId);
            }
        }

        //This function is meant for the pixmap cache
        QString cachePath(const QString &path, const QSize &size)
        {
             return QString("%3_%2_%1_").arg(int(size.height()))
                                       .arg(int(size.width()))
                                       .arg(path);
        }

        bool setImagePath(const QString &imagePath, Svg *q)
        {
            bool isThemed = !QDir::isAbsolutePath(imagePath);

            // lets check to see if we're already set to this file
            if (isThemed == themed &&
                ((themed && themePath == imagePath) ||
                 (!themed && path == imagePath))) {
                return false;
            }

            // if we don't have any path right now and are going to set one,
            // then lets not schedule a repaint because we are just initializing!
            bool updateNeeded = true; //!path.isEmpty() || !themePath.isEmpty();

            if (themed) {
                QObject::disconnect(Plasma::Theme::defaultTheme(), SIGNAL(themeChanged()),
                                    q, SLOT(themeChanged()));
                QObject::disconnect(KGlobalSettings::self(), SIGNAL(kdisplayPaletteChanged()),
                                    q, SLOT(colorsChanged()));
            }

            themed = isThemed;
            path.clear();
            themePath.clear();

            if (themed) {
                themePath = imagePath;
                QObject::connect(Plasma::Theme::defaultTheme(), SIGNAL(themeChanged()),
                                 q, SLOT(themeChanged()));

                // check if svg wants colorscheme applied
                checkApplyColorHint();
                if (applyColors && !Theme::defaultTheme()->colorScheme()) {
                    QObject::connect(KGlobalSettings::self(), SIGNAL(kdisplayPaletteChanged()),
                                     q, SLOT(colorsChanged()));
                }
            } else if (QFile::exists(imagePath)) {
                path = imagePath;
            } else {
                kDebug() << "file '" << path << "' does not exist!";
            }

            return updateNeeded;
        }

        QPixmap findInCache(const QString &elementId, const QSizeF &s = QSizeF())
        {
            QSize size;
            if (elementId.isEmpty() || (multipleImages && s.isValid())) {
                size = s.toSize();
            } else {
                size = elementRect(elementId).size().toSize();
            }

            if (size.isEmpty()) {
                return QPixmap();
            }

            QString id = cachePath(path, size);

            if (!elementId.isEmpty()) {
                id.append(elementId);
            }

            //kDebug() << "id is " << id;

            Theme *theme = Theme::defaultTheme();
            QPixmap p;

            if (theme->findInCache(id, p)) {
                //kDebug() << "found cached version of " << id << p.size();
                return p;
            } else {
                //kDebug() << "didn't find cached version of " << id << ", so re-rendering";
            }

            //kDebug() << "size for " << elementId << " is " << s;
            // we have to re-render this puppy

            p = QPixmap(size);

            p.fill(Qt::transparent);
            QPainter renderPainter(&p);

            createRenderer();
            if (elementId.isEmpty()) {
                renderer->render(&renderPainter);
            } else {
                renderer->render(&renderPainter, elementId);
            }

            renderPainter.end();

            // Apply current color scheme if the svg asks for it
            if (applyColors) {
                QImage itmp = p.toImage();
                KIconEffect::colorize(itmp, theme->color(Theme::BackgroundColor), 1.0);
                p = p.fromImage(itmp);
            }

            if (!itemsToSave.contains(id)) {
                itemsToSave.insert(id, p);
                saveTimer->start(300);
            }

            return p;
        }

        void scheduledCacheUpdate()
        {
            QHash<QString, QPixmap>::const_iterator i = itemsToSave.constBegin();

            while (i != itemsToSave.constEnd()) {
                //kDebug()<<"Saving item to cache: "<<i.key();
                Theme::defaultTheme()->insertIntoCache(i.key(), i.value());
                ++i;
            }

            itemsToSave.clear();
        }

        void createRenderer()
        {
            if (renderer) {
                return;
            }

            //kDebug() << kBacktrace();
            if (themed && path.isEmpty()) {
                Applet *applet = qobject_cast<Applet*>(q->parent());
                if (applet && applet->package()) {
                    path = applet->package()->filePath("images", themePath + ".svg");

                    if (path.isEmpty()) {
                        path = applet->package()->filePath("images", themePath + ".svgz");
                    }
                }

                if (path.isEmpty()) {
                    path = Plasma::Theme::defaultTheme()->imagePath(themePath);
                }
            }

            //kDebug() << "********************************";
            //kDebug() << "FAIL! **************************";
            //kDebug() << path << "**";

            QHash<QString, SharedSvgRenderer::Ptr>::const_iterator it = s_renderers.constFind(path);

            if (it != s_renderers.constEnd()) {
                //kDebug() << "gots us an existing one!";
                renderer = it.value();
            } else {
                renderer = new SharedSvgRenderer(path);
                s_renderers[path] = renderer;
            }

            if (size == QSizeF()) {
                size = renderer->defaultSize();
            }
        }

        void eraseRenderer()
        {
            if (renderer && renderer.count() == 2) {
                // this and the cache reference it
                s_renderers.erase(s_renderers.find(path));
                Plasma::Theme::defaultTheme()->releaseRectsCache(path);
            }

            renderer = 0;
            localRectCache.clear();
        }

        QRectF elementRect(const QString &elementId)
        {
            if (themed && path.isEmpty()) {
                path = Plasma::Theme::defaultTheme()->imagePath(themePath);
            }

            QString id = cacheId(elementId);
            if (localRectCache.contains(id)) {
                return localRectCache.value(id);
            }

            QRectF rect;
            bool found = Theme::defaultTheme()->findInRectsCache(path, id, rect);

            if (found) {
                localRectCache.insert(id, rect);
                return rect;
            }

            return findAndCacheElementRect(elementId);
        }

        QRectF findAndCacheElementRect(const QString &elementId)
        {
            createRenderer();
            QRectF elementRect = renderer->elementExists(elementId) ?
                                 renderer->boundsOnElement(elementId) : QRectF();
            naturalSize = renderer->defaultSize();
            qreal dx = size.width() / naturalSize.width();
            qreal dy = size.height() / naturalSize.height();

            elementRect = QRectF(elementRect.x() * dx, elementRect.y() * dy,
                                 elementRect.width() * dx, elementRect.height() * dy);
            Theme::defaultTheme()->insertIntoRectsCache(path, cacheId(elementId), elementRect);

            return elementRect;
        }

        QMatrix matrixForElement(const QString &elementId)
        {
            createRenderer();
            return renderer->matrixForElement(elementId);
        }

        void checkApplyColorHint()
        {
            QRectF elementRect;
            bool found = Theme::defaultTheme()->findInRectsCache(themePath, cacheId("hint-apply-color-scheme"), elementRect);

            if (found) {
                applyColors = elementRect.isValid();
            } else {
                findAndCacheElementRect("hint-apply-color-scheme");
                applyColors = renderer->elementExists("hint-apply-color-scheme");
            }
        }

        void themeChanged()
        {
            if (!themed) {
                return;
            }

            QString newPath = Theme::defaultTheme()->imagePath(themePath);

            if (path == newPath) {
                return;
            }

            path = newPath;
            //delete d->renderer; we're a KSharedPtr
            eraseRenderer();

            // check if new theme svg wants colorscheme applied
            bool wasApplyColors = applyColors;
            checkApplyColorHint();
            if (applyColors && !Theme::defaultTheme()->colorScheme()) {
                if (!wasApplyColors) {
                    QObject::connect(KGlobalSettings::self(), SIGNAL(kdisplayPaletteChanged()),
                                     q, SLOT(colorsChanged()));
                }
            } else {
                QObject::disconnect(KGlobalSettings::self(), SIGNAL(kdisplayPaletteChanged()),
                                    q, SLOT(colorsChanged()));
            }

            localRectCache.clear();
            itemsToSave.clear();
            saveTimer->stop();

            //kDebug() << themePath << ">>>>>>>>>>>>>>>>>> theme changed";
            emit q->repaintNeeded();
        }

        void colorsChanged()
        {
            if (!applyColors) {
                return;
            }

            eraseRenderer();
            itemsToSave.clear();
            saveTimer->stop();
            emit q->repaintNeeded();
        }

        Svg *q;
        static QHash<QString, SharedSvgRenderer::Ptr> s_renderers;
        QHash<QString, QRectF> localRectCache;
        QHash<QString, QPixmap> itemsToSave;
        QTimer *saveTimer;
        SharedSvgRenderer::Ptr renderer;
        QString themePath;
        QString path;
        QSizeF size;
        QSizeF naturalSize;
        bool multipleImages;
        bool themed;
        bool applyColors;
};

QHash<QString, SharedSvgRenderer::Ptr> SvgPrivate::s_renderers;

Svg::Svg(QObject *parent)
    : QObject(parent),
      d(new SvgPrivate(this))
{
    d->saveTimer = new QTimer(this);
    d->saveTimer->setSingleShot(true);
    connect(d->saveTimer, SIGNAL(timeout()), this, SLOT(scheduledCacheUpdate()));
}

Svg::~Svg()
{
    delete d;
}

QPixmap Svg::pixmap(const QString &elementID)
{
    if (elementID.isNull() || d->multipleImages) {
        return d->findInCache(elementID, size());
    } else {
        return d->findInCache(elementID);
    }
}

void Svg::paint(QPainter *painter, const QPointF &point, const QString &elementID)
{
    QPixmap pix(elementID.isNull() ? d->findInCache(elementID, size()) :
                                     d->findInCache(elementID));

    if (pix.isNull()) {
        return;
    }

    painter->drawPixmap(QRectF(point, pix.size()), pix, QRectF(QPointF(0, 0), pix.size()));
}

void Svg::paint(QPainter *painter, int x, int y, const QString &elementID)
{
    paint(painter, QPointF(x, y), elementID);
}

void Svg::paint(QPainter *painter, const QRectF &rect, const QString &elementID)
{
    QPixmap pix(d->findInCache(elementID, rect.size()));
    painter->drawPixmap(rect, pix, QRectF(QPointF(0, 0), pix.size()));
}

void Svg::paint(QPainter *painter, int x, int y, int width, int height, const QString &elementID)
{
    QPixmap pix(d->findInCache(elementID, QSizeF(width, height)));
    painter->drawPixmap(x, y, pix, 0, 0, pix.size().width(), pix.size().height());
}

QSize Svg::size() const
{
    if (d->size.isEmpty()) {
        d->createRenderer();
        d->size = d->renderer->defaultSize();
    }

    return d->size.toSize();
}

void Svg::resize(qreal width, qreal height)
{
    resize(QSize(width, height));
}

void Svg::resize(const QSizeF &size)
{
    if (qFuzzyCompare(size.width(), d->size.width()) &&
        qFuzzyCompare(size.height(), d->size.height())) {
        return;
    }

    d->size = size;
    d->localRectCache.clear();
}

void Svg::resize()
{
    if (d->naturalSize.isEmpty()) {
        d->createRenderer();
        d->naturalSize = d->renderer->defaultSize();
    }


    if (qFuzzyCompare(d->naturalSize.width(), d->size.width()) &&
        qFuzzyCompare(d->naturalSize.height(), d->size.height())) {
        return;
    }

    d->size = d->naturalSize;
    d->localRectCache.clear();
}

QSize Svg::elementSize(const QString &elementId) const
{
    return d->elementRect(elementId).size().toSize();
}

QRectF Svg::elementRect(const QString &elementId) const
{
    return d->elementRect(elementId);
}

bool Svg::hasElement(const QString &elementId) const
{
    if (d->path.isNull() && d->themePath.isNull()) {
        return false;
    }

    QString id = d->cacheId(elementId);
    if (d->localRectCache.contains(id)) {
        return d->localRectCache.value(id).isValid();
    }

    QRectF elementRect;
    bool found = Theme::defaultTheme()->findInRectsCache(d->path, id, elementRect);

    if (found) {
        d->localRectCache.insert(id, elementRect);
        return elementRect.isValid();
    } else {
//        kDebug() << "** ** *** !!!!!!!! *** ** ** creating renderer due to hasElement miss" << d->path << elementId;
        return d->findAndCacheElementRect(elementId).isValid();
    }
}

QString Svg::elementAtPoint(const QPoint &point) const
{
    Q_UNUSED(point)

    return QString();
/*
FIXME: implement when Qt can support us!
    d->createRenderer();
    QSizeF naturalSize = d->renderer->defaultSize();
    qreal dx = d->size.width() / naturalSize.width();
    qreal dy = d->size.height() / naturalSize.height();
    //kDebug() << point << "is really"
    //         << QPoint(point.x() *dx, naturalSize.height() - point.y() * dy);

    return QString(); // d->renderer->elementAtPoint(QPoint(point.x() *dx, naturalSize.height() - point.y() * dy));
    */
}

bool Svg::isValid() const
{
    if (d->path.isNull() && d->themePath.isNull()) {
        return false;
    }

    d->createRenderer();
    return d->renderer->isValid();
}

void Svg::setContainsMultipleImages(bool multiple)
{
    d->multipleImages = multiple;
}

bool Svg::containsMultipleImages() const
{
    return d->multipleImages;
}

void Svg::setImagePath(const QString &svgFilePath)
{
    d->setImagePath(svgFilePath, this);
    d->eraseRenderer();
    emit repaintNeeded();
}

QString Svg::imagePath() const
{
   return d->themed ? d->themePath : d->path;
}

} // Plasma namespace

#include "svg.moc"

