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

#ifndef KATE_SCRIPT_DOCUMENT_H
#define KATE_SCRIPT_DOCUMENT_H

#include <QObject>
#include <QtScript/QScriptable>
#include <QtScript/QScriptValue>

#include <ktexteditor/cursor.h>

class KateDocument;

/**
 * Thinish wrapping around KateDocument, exposing the methods we want exposed
 * and adding some helper methods.
 *
 * We inherit from QScriptable to have more thight access to the scripting
 * engine.
 *
 * setDocument _must_ be called before using any other method. This is not checked
 * for the sake of speed.
 */
class KateScriptDocument : public QObject, protected QScriptable
{
  /// Properties are accessible with a nicer syntax from JavaScript
  Q_OBJECT
  Q_PROPERTY(QString fileName READ fileName)
  Q_PROPERTY(QString url READ url)
  Q_PROPERTY(QString mimeType READ mimeType)
  Q_PROPERTY(QString encoding READ encoding)
  Q_PROPERTY(bool modified READ isModified)
  Q_PROPERTY(QString text READ text WRITE setText)

  public:
    KateScriptDocument(QObject *parent=0);
    void setDocument(KateDocument *document);
    KateDocument *document();
    // XX Automatically generated from the KJS document wrapper. Feel free to
    // add descriptive variable names ;)
    //BEGIN
    Q_INVOKABLE QString fileName();
    Q_INVOKABLE QString url();
    Q_INVOKABLE QString mimeType();
    Q_INVOKABLE QString encoding();
    Q_INVOKABLE bool isModified();
    Q_INVOKABLE QString text();
    Q_INVOKABLE QString textRange(int i, int j, int k, int l);
    Q_INVOKABLE QString line(int i);
    Q_INVOKABLE QString wordAt(int i, int j);
    Q_INVOKABLE QString charAt(int i, int j);
    Q_INVOKABLE QString firstChar(int i);
    Q_INVOKABLE QString lastChar(int i);
    Q_INVOKABLE bool isSpace(int i, int j);
    Q_INVOKABLE bool matchesAt(int i, int j, const QString &s);
    Q_INVOKABLE bool setText(const QString &s);
    Q_INVOKABLE bool clear();
    Q_INVOKABLE bool truncate(int i, int j);
    Q_INVOKABLE bool insertText(int i, int j, const QString &s);
    Q_INVOKABLE bool removeText(int i, int j, int k, int l);
    Q_INVOKABLE bool insertLine(int i, const QString &s);
    Q_INVOKABLE bool removeLine(int i);
    Q_INVOKABLE void joinLines(int i, int j);
    Q_INVOKABLE int lines();
    Q_INVOKABLE int length();
    Q_INVOKABLE int lineLength(int i);
    Q_INVOKABLE void editBegin();
    Q_INVOKABLE void editEnd();
    Q_INVOKABLE int firstColumn(int i);
    Q_INVOKABLE int lastColumn(int i);
    Q_INVOKABLE int prevNonSpaceColumn(int i, int j);
    Q_INVOKABLE int nextNonSpaceColumn(int i, int j);
    Q_INVOKABLE int prevNonEmptyLine(int i);
    Q_INVOKABLE int nextNonEmptyLine(int i);
    Q_INVOKABLE bool isInWord(const QString &s, int i);
    Q_INVOKABLE bool canBreakAt(const QString &s, int i);
    Q_INVOKABLE bool canComment(int i, int j);
    Q_INVOKABLE QString commentMarker(int i);
    Q_INVOKABLE QString commentStart(int i);
    Q_INVOKABLE QString commentEnd(int i);

    /**
     * Get the syntax highlighting attribute at a given position in the document.
     */
    Q_INVOKABLE int attribute(int line, int column);

    /**
     * Return true if the highlight attribute equals @p attr.
     */
    Q_INVOKABLE bool isAttribute(int line, int column, int attr);

    /**
     * Get the name of the syntax highlighting attribute at the given position.
     */
    Q_INVOKABLE QString attributeName(int line, int column);

    /**
     * Return true is the name of the syntax attribute equals @p name.
     */
    Q_INVOKABLE bool isAttributeName(int line, int column, const QString &name);

    Q_INVOKABLE QString variable(const QString &s);
    //END

    Q_INVOKABLE int firstVirtualColumn(int line);
    Q_INVOKABLE int lastVirtualColumn(int line);
    Q_INVOKABLE int toVirtualColumn(int line, int column);
    Q_INVOKABLE int fromVirtualColumn(int line, int virtualColumn);

    Q_INVOKABLE KTextEditor::Cursor anchor(int line, int column, QChar character);
    Q_INVOKABLE QScriptValue rfind(int line, int column, const QString& text, int attribute = -1);

    Q_INVOKABLE int defStyleNum(int line, int column);
    Q_INVOKABLE bool isCode(int line, int column);
    Q_INVOKABLE bool isComment(int line, int column);
    Q_INVOKABLE bool isString(int line, int column);
    Q_INVOKABLE bool isRegionMarker(int line, int column);
    Q_INVOKABLE bool isChar(int line, int column);
    Q_INVOKABLE bool isOthers(int line, int column);


    Q_INVOKABLE bool startsWith (int line, const QString &pattern, bool skipWhiteSpaces);
    Q_INVOKABLE bool endsWith (int line, const QString &pattern, bool skipWhiteSpaces);

  private:
    bool _isCode(int defaultStyle);

    KateDocument *m_document;
};


#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
