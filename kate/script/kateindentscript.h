/// This file is part of the KDE libraries
/// Copyright (C) 2008 Paul Giannaros <paul@giannaros.org>
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

#ifndef KATE_INDENT_SCRIPT_H
#define KATE_INDENT_SCRIPT_H

#include "katescript.h"
#include "kateview.h"

#include <QtCore/QPair>

class KateScriptDocument;

/**
 * A specialized class for scripts that are of type 
 * KateScriptInformation::IndentationScript
 */
class KateIndentScript : public KateScript {
  public:
    KateIndentScript(const QString &url, const KateScriptInformation &information);

    const QString &triggerCharacters();

    /**
     * Returns a pair where the first value is the indent amount, and the second
     * value is the alignment.
     */
    QPair<int, int> indent(KateView* view, const KTextEditor::Cursor& position, QChar typedCharacter,
                           int indentWidth);

  private:
    QString m_triggerCharacters;
    bool m_triggerCharactersSet;
};


#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
