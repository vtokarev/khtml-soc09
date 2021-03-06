/* This file is part of the KDE libraries
    Copyright (C) 2005, 2006 Ian Reinhart Geiser <geiseri@kde.org>
    Copyright (C) 2005, 2006 Matt Broadstone <mbroadst@gmail.com>
    Copyright (C) 2005, 2006 Richard J. Moore <rich@kde.org>
    Copyright (C) 2005, 2006 Erik L. Bunce <kde@bunce.us>

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
#ifndef CONSOLE_H
#define CONSOLE_H
#include <QtGui/QMainWindow>
#include <QtGui/QStandardItem>

#include "kjsembed.h"

class KJSObjectModel;

class Console : public QMainWindow
{
    Q_OBJECT
    public:
    Console( QWidget *parent = 0);
    ~Console();

    public Q_SLOTS:
        void on_mExecute_clicked();
        void on_actionOpenScript_activated();
        void on_actionCloseScript_activated();
        void on_actionQuit_activated();
        void on_actionRun_activated();
        void on_actionRunTo_activated();
        void on_actionStep_activated();
        void on_actionStop_activated();

    private:
        void updateModel(const QModelIndex &parent, KJS::Object &obj );

        KJSEmbed::Engine mKernel;
        KJSObjectModel *m_model;
        QModelIndex m_root;
};
#endif
