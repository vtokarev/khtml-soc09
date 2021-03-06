/*
 *   Copyright 2008 Aaron Seigo <aseigo@kde.org>
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

#include "textedit.h"

#include <QPainter>
#include <QScrollBar>

#include <kmimetype.h>
#include <ktextedit.h>

#include "theme.h"
#include "svg.h"
#include "private/style_p.h"

namespace Plasma
{

class TextEditPrivate
{
public:
    Plasma::Style::Ptr style;
};

TextEdit::TextEdit(QGraphicsWidget *parent)
    : QGraphicsProxyWidget(parent),
      d(new TextEditPrivate)
{
    KTextEdit *native = new KTextEdit;
    connect(native, SIGNAL(textChanged()), this, SIGNAL(textChanged()));
    setWidget(native);
    native->setAttribute(Qt::WA_NoSystemBackground);
    native->setFrameShape( QFrame::NoFrame );
    native->setTextBackgroundColor( Qt::transparent );
    native->viewport()->setAutoFillBackground( false );
    d->style = Plasma::Style::sharedStyle();
    native->verticalScrollBar()->setStyle(d->style.data());
    native->horizontalScrollBar()->setStyle(d->style.data());
}

TextEdit::~TextEdit()
{
    delete d;
    Plasma::Style::doneWithSharedStyle();
}

void TextEdit::setText(const QString &text)
{
    //FIXME I'm not certain about using only the html methods. look at this again later.
    static_cast<KTextEdit*>(widget())->setHtml(text);
}

QString TextEdit::text() const
{
    return static_cast<KTextEdit*>(widget())->toHtml();
}

void TextEdit::setStyleSheet(const QString &stylesheet)
{
    widget()->setStyleSheet(stylesheet);
}

QString TextEdit::styleSheet()
{
    return widget()->styleSheet();
}

KTextEdit *TextEdit::nativeWidget() const
{
    return static_cast<KTextEdit*>(widget());
}

void TextEdit::dataUpdated(const QString &sourceName, const Plasma::DataEngine::Data &data)
{
    Q_UNUSED(sourceName)

    KTextEdit *te = nativeWidget();
    te->clear();

    foreach (const QVariant &v, data) {
        if (v.canConvert(QVariant::String)) {
            te->append(v.toString() + '\n');
        }
    }
}

void TextEdit::resizeEvent(QGraphicsSceneResizeEvent *event)
{
    QGraphicsProxyWidget::resizeEvent(event);
}

} // namespace Plasma

#include <textedit.moc>

