/*
    This file is part of KNewStuff2.
    Copyright (c) 2006, 2007 Josef Spillner <spillner@kde.org>

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

#ifndef KNEWSTUFF2_UI_QNOWTOOLTIP_H
#define KNEWSTUFF2_UI_QNOWTOOLTIP_H

#include <QtCore/QRect>
#include <QtCore/QString>
#include <QtGui/QWidget>

/**
 * Convenience class for tooltips.
 *
 * FIXME: why was this introduced?
 *
 * This class is used internally by the KDXSComments class.
 *
 * @internal
 */
class QNowToolTip
{
public:
    static void display(QWidget *parent, const QRect& pos, const QString& text);
};

#endif

