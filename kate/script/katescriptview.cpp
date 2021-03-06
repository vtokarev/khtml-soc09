/// This file is part of the KDE libraries
/// Copyright (C) 2008 Paul Giannaros <paul@giannaros.org>
/// Copyright (C) 2008 Christoph Cullmann <cullmann@kde.org>
///
/// This library is free software; you can redistribute it and/or
/// modify it under the terms of the GNU Library General Public
/// License as published by the Free Software Foundation; either
/// version 2 of the License, or (at your option) version 3.
///
/// This library is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
/// Library General Public License for more details.
///
/// You should have received a copy of the GNU Library General Public License
/// along with this library; see the file COPYING.LIB.  If not, write to
/// the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
/// Boston, MA 02110-1301, USA.

#include "katescriptview.h"

#include "katedocument.h"
#include "kateview.h"
#include "katerenderer.h"
#include "katehighlight.h"
#include "katescript.h"

KateScriptView::KateScriptView(QObject *parent)
  : QObject(parent), m_view(0)
{
}

void KateScriptView::setView (KateView *view)
{
  m_view = view;
}

KateView *KateScriptView::view()
{
  return m_view;
}

KTextEditor::Cursor KateScriptView::cursorPosition ()
{
  return m_view->cursorPosition();
}

// kate: space-indent on; indent-width 2; replace-tabs on;

