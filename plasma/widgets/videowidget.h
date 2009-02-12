/*
 *   Copyright 2009 Marco Martin <notmart@gmail.com>
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

#ifndef PLASMA_VIDEOWIDGET_H
#define PLASMA_VIDEOWIDGET_H

#include <QtGui/QGraphicsProxyWidget>

#include <plasma/plasma_export.h>

namespace Phonon {
    class VideoWidget;
    class MediaObject;
    class AudioOutput;
}

class KUrl;

namespace Plasma
{

class VideoWidgetPrivate;

/**
 * @class VideoWidget plasma/widgets/videowidget.h <Plasma/Widgets/VideoWidget>
 * a Video playing widget via Phonon, it encloses the
 * Phonon::MediaObject and Phonon::AudioOutput too
 *
 * @short Provides a video player widget
 * @since KDE4.3
 */
class PLASMA_EXPORT VideoWidget : public QGraphicsProxyWidget
{
    Q_OBJECT

    Q_PROPERTY(QString url READ url WRITE setUrl)
    Q_PROPERTY(QString currentTime READ currentTime)
    Q_PROPERTY(QString totalTime READ totalTime)
    Q_PROPERTY(QString remainingTime READ remainingTime)
    Q_PROPERTY(Controls shownControls READ shownControls WRITE setShownControls)
    Q_PROPERTY(QString styleSheet READ styleSheet WRITE setStyleSheet)

public:
    enum Control {
        NoControls = 0,
        Play = 1,
        Pause = 2,
        Stop = 4,
        PlayPause = 8,
        Progress = 32,
        Volume = 64,
        OpenFile = 128,
        DefaultControls = PlayPause|Progress|Volume|OpenFile
    };
    Q_DECLARE_FLAGS(Controls, Control);

    explicit VideoWidget(QGraphicsWidget *parent = 0);
    ~VideoWidget();

    /**
     * Load a certain url that can be a local file or a remote one
     * @arg path resource to play
     */
    void setUrl(const QString &url);

    /**
     * @return the url (local or remote) we are playing
     */
    QString url() const;

    /**
     * @return the Phonon::MediaObject being used
     * @see Phonon::MediaObject
     */
    Q_INVOKABLE Phonon::MediaObject *mediaObject() const;

    /**
     * @return the Phonon::AudioOutput being used
     * @see Phonon::AudioOutput
     */
    Q_INVOKABLE Phonon::AudioOutput *audioOutput() const;

    /**
     * @return the current time of the current media file
     */
    qint64 currentTime() const;

    /**
     * @return the total playing time of the current media file
     */
    qint64 totalTime() const;

    /**
     * @return the time remaining to the current media file
     */
    qint64 remainingTime() const;

    /**
     * Set what control widgets to show
     *
     * @arg controls OR combination of Controls flags
     * @see Controls
     */
    void setShownControls(Controls controls);

    /**
     * @return the video controls that are being show right now
     * @see Controls
     */
    Controls shownControls() const;

    /**
     * Sets the stylesheet used to control the visual display of this VideoWidget
     *
     * @arg stylesheet a CSS string
     */
    void setStyleSheet(const QString &stylesheet);

    /**
     * @return the stylesheet currently used with this widget
     */
    QString styleSheet();

    /**
     * @return the native widget wrapped by this VideoWidget
     */
    Phonon::VideoWidget *nativeWidget() const;

public Q_SLOTS:
    /**
     * Play the current file
     */
    void play();

    /**
     * Pause the current file
     */
    void pause();

    /**
     * Stop the current file
     */
    void stop();

    /**
     * Jump at a given millisecond in the current file
     * @arg time where we want to jump
     */
    void seek(qint64 time);

Q_SIGNALS:
    /**
     * Emitted regularly when the playing is progressing
     * @arg time where we are
     */
    void tick(qint64 time);

    /**
     * Emitted an instant before the playback is finished
     */
    void aboutToFinish();

protected:
    void resizeEvent(QGraphicsSceneResizeEvent *event);

private:
    VideoWidgetPrivate * const d;

    Q_PRIVATE_SLOT(d, void playPause())
    Q_PRIVATE_SLOT(d, void ticked(qint64 progress))
    Q_PRIVATE_SLOT(d, void totalTimeChanged(qint64 time))
    Q_PRIVATE_SLOT(d, void setPosition(int progress))
    Q_PRIVATE_SLOT(d, void setVolume(int value))
    Q_PRIVATE_SLOT(d, void volumeChanged(qreal value))
    Q_PRIVATE_SLOT(d, void showOpenFileDialog())
    Q_PRIVATE_SLOT(d, void stateChanged(Phonon::State newState, Phonon::State oldState))
};

} // namespace Plasma

#endif // multiple inclusion guard
