/***************************************************************************
 *   Copyright (C) 2006 by Aaron J. Seigo (<aseigo@kde.org>)               *
 *   Copyright (C) 2009 by Peter Penz (<peter.penz@kde.org>)               *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Lesser General Public            *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with this library; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA          *
 ***************************************************************************/

#include "kprotocolcombo_p.h"

#include <QtGui/QAction>
#include <QtGui/QMenu>
#include <QtGui/QPainter>
#include <QtGui/QPaintEvent>
#include <QtGui/QStyleOption>

#include <klocale.h>
#include <kprotocolinfo.h>
#include <kprotocolmanager.h>
#include <kurlnavigator.h>

const int ARROW_SIZE = 10;

KProtocolCombo::KProtocolCombo(const QString& protocol, KUrlNavigator* parent) :
    KUrlButton(parent),
    m_menu(0),
    m_protocols(),
    m_categories()
{
    m_menu = new QMenu(this);
    connect(m_menu, SIGNAL(triggered(QAction*)), this, SLOT(setProtocol(QAction*)));
    setText(protocol);
    setMenu(m_menu);
}

void KProtocolCombo::setCustomProtocols(const QStringList& protocols)
{
    m_protocols = protocols;
    m_menu->clear();

    foreach (const QString& protocol, protocols) {
        QAction* action = m_menu->addAction(protocol);
        action->setData(protocol);
    }
}

QSize KProtocolCombo::sizeHint() const
{
    QSize size = KUrlButton::sizeHint();

    QFontMetrics fontMetrics(font());
    int width = fontMetrics.width(text());
    width += (3 * BorderWidth) + ARROW_SIZE;

    return QSize(width, size.height());
}

void KProtocolCombo::setProtocol(const QString& protocol)
{
    setText(protocol);
}

QString KProtocolCombo::currentProtocol() const
{
    return text();
}

void KProtocolCombo::showEvent(QShowEvent* event)
{
    KUrlButton::showEvent(event);
    if (!event->spontaneous() && m_protocols.isEmpty()) {
        m_protocols = KProtocolInfo::protocols();
        qSort(m_protocols);

        QStringList::iterator it = m_protocols.begin();
        while (it != m_protocols.end()) {
            const KUrl url(*it + "://");
            if (!KProtocolManager::supportsListing(url)) {
                it = m_protocols.erase(it);
            } else {
                ++it;
            }
        }

        updateMenu();
    }
}

void KProtocolCombo::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    const int buttonWidth  = width();
    const int buttonHeight = height();

    drawHoverBackground(&painter);

    const QColor fgColor = foregroundColor();
    painter.setPen(fgColor);

    // draw arrow
    const int arrowX = buttonWidth - ARROW_SIZE - BorderWidth;
    const int arrowY = (buttonHeight - ARROW_SIZE) / 2;

    QStyleOption option;
    option.rect = QRect(arrowX, arrowY, ARROW_SIZE, ARROW_SIZE);
    option.palette = palette();
    option.palette.setColor(QPalette::Text, fgColor);
    option.palette.setColor(QPalette::WindowText, fgColor);
    option.palette.setColor(QPalette::ButtonText, fgColor);
    style()->drawPrimitive(QStyle::PE_IndicatorArrowDown, &option, &painter, this );

    // draw text
    const int textWidth = arrowX - (2 * BorderWidth);
    painter.drawText(QRect(BorderWidth, 0, textWidth, buttonHeight), Qt::AlignCenter, text());
}

void KProtocolCombo::setProtocol(QAction* action)
{
    const QString protocol = action->data().toString();
    setText(protocol);
    emit activated(protocol);
}

void KProtocolCombo::updateMenu()
{
    initializeCategories();
    qSort(m_protocols);

    // move all protocols into the corresponding category of 'items'
    QList<QString> items[CategoryCount];
    foreach (const QString& protocol, m_protocols) {
        if (m_categories.contains(protocol)) {
            const ProtocolCategory category = m_categories.value(protocol);
            items[category].append(protocol);
        } else {
            items[OtherCategory].append(protocol);
        }
    }

    // Create the menu that includes all entries from 'items'. The categories
    // CoreCategory and PlacesCategory are placed at the top level, the remaining
    // categories are placed in sub menus.
    QMenu* menu = m_menu;
    for (int category = 0; category < CategoryCount; ++category) {
        if (items[category].count() > 0) {
            switch (category) {
            case DevicesCategory:
                menu = m_menu->addMenu(i18nc("@item:inmenu", "Devices"));
                break;

            case SubversionCategory:
                menu = m_menu->addMenu(i18nc("@item:inmenu", "Subversion"));
                break;

            case OtherCategory:
                menu = m_menu->addMenu(i18nc("@item:inmenu", "Other"));
                break;

            case CoreCategory:
            case PlacesCategory:
            default:
                break;
            }

            foreach (const QString& protocol, items[category]) {
                QAction* action = menu->addAction(protocol);
                action->setData(protocol);
            }

            if (menu == m_menu) {
                menu->addSeparator();
            }
        }
    }
}

void KProtocolCombo::initializeCategories()
{
    if (m_categories.isEmpty()) {
        m_categories.insert("file", CoreCategory);
        m_categories.insert("ftp", CoreCategory);
        m_categories.insert("fish", CoreCategory);
        m_categories.insert("nfs", CoreCategory);
        m_categories.insert("sftp", CoreCategory);
        m_categories.insert("smb", CoreCategory);
        m_categories.insert("webdav", CoreCategory);

        m_categories.insert("desktop", PlacesCategory);
        m_categories.insert("fonts", PlacesCategory);
        m_categories.insert("programs", PlacesCategory);
        m_categories.insert("settings", PlacesCategory);
        m_categories.insert("trash", PlacesCategory);

        m_categories.insert("floppy", DevicesCategory);
        m_categories.insert("camera", DevicesCategory);
        m_categories.insert("remote", DevicesCategory);

        m_categories.insert("svn", SubversionCategory);
        m_categories.insert("svn+file", SubversionCategory);
        m_categories.insert("svn+http", SubversionCategory);
        m_categories.insert("svn+https", SubversionCategory);
        m_categories.insert("svn+ssh", SubversionCategory);
    }
}

#include "kprotocolcombo_p.moc"
