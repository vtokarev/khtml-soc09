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

#include "katescriptdocument.h"

#include "katedocument.h"
#include "kateview.h"
#include "katerenderer.h"
#include "katehighlight.h"
#include "katescript.h"

#include <QtScript/QScriptEngine>

KateScriptDocument::KateScriptDocument(QObject *parent)
  : QObject(parent), m_document(0)
{
}

void KateScriptDocument::setDocument(KateDocument *document)
{
  m_document = document;
}

KateDocument *KateScriptDocument::document()
{
  return m_document;
}

int KateScriptDocument::defStyleNum(int line, int column)
{
  KateDocCursor cursor(line, column, m_document);
  QList<KTextEditor::Attribute::Ptr> attributes = m_document->highlight()->attributes(((KateView*) m_document->activeView())->renderer()->config()->schema());
  KTextEditor::Attribute::Ptr a = attributes[cursor.currentAttrib()];
  return a->property(KateExtendedAttribute::AttributeDefaultStyleIndex).toInt();
}

bool KateScriptDocument::isCode(int line, int column)
{
  const int defaultStyle = defStyleNum(line, column);
  return _isCode(defaultStyle);
}

bool KateScriptDocument::isComment(int line, int column)
{
  const int defaultStyle = defStyleNum(line, column);
  return defaultStyle == KateExtendedAttribute::dsComment;
}

bool KateScriptDocument::isString(int line, int column)
{
  const int defaultStyle = defStyleNum(line, column);
  return defaultStyle == KateExtendedAttribute::dsString;
}

bool KateScriptDocument::isRegionMarker(int line, int column)
{
  const int defaultStyle = defStyleNum(line, column);
  return defaultStyle == KateExtendedAttribute::dsRegionMarker;
}

bool KateScriptDocument::isChar(int line, int column)
{
  const int defaultStyle = defStyleNum(line, column);
  return defaultStyle == KateExtendedAttribute::dsChar;
}

bool KateScriptDocument::isOthers(int line, int column)
{
  const int defaultStyle = defStyleNum(line, column);
  return defaultStyle == KateExtendedAttribute::dsOthers;
}

int KateScriptDocument::firstVirtualColumn(int line)
{
  const int tabWidth = m_document->config()->tabWidth();
  KateTextLine::Ptr textLine = m_document->plainKateTextLine(line);
  const int firstPos = textLine ? textLine->firstChar() : -1;
  if(!textLine || firstPos == -1)
    return -1;
  return textLine->indentDepth(tabWidth);
}

int KateScriptDocument::lastVirtualColumn(int line)
{
  const int tabWidth = m_document->config()->tabWidth();
  KateTextLine::Ptr textLine = m_document->plainKateTextLine(line);
  const int lastPos = textLine ? textLine->lastChar() : -1;
  if(!textLine || lastPos == -1)
    return -1;
  return textLine->toVirtualColumn(lastPos, tabWidth);
}

int KateScriptDocument::toVirtualColumn(int line, int column)
{
  const int tabWidth = m_document->config()->tabWidth();
  KateTextLine::Ptr textLine = m_document->plainKateTextLine(line);
  if (!textLine || column < 0 || column > textLine->length()) return -1;
  return textLine->toVirtualColumn(column, tabWidth);
}

int KateScriptDocument::fromVirtualColumn(int line, int virtualColumn)
{
  const int tabWidth = m_document->config()->tabWidth();
  KateTextLine::Ptr textLine = m_document->plainKateTextLine(line);
  if(!textLine || virtualColumn < 0 || virtualColumn > textLine->virtualLength(tabWidth))
    return -1;
  return textLine->fromVirtualColumn(virtualColumn, tabWidth);
}

QScriptValue KateScriptDocument::rfind(int line, int column, const QString& text, int attribute)
{
  KateDocCursor cursor(line, column, m_document);
  const int start = cursor.line();
  QList<KTextEditor::Attribute::Ptr> attributes =
      m_document->highlight()->attributes(((KateView*)m_document->activeView())->renderer()->config()->schema());

  do {
    KateTextLine::Ptr textLine = m_document->plainKateTextLine(cursor.line());
    if (!textLine)
      break;

    if (cursor.line() != start) {
      cursor.setColumn(textLine->length());
    } else if (column >= textLine->length()) {
      cursor.setColumn(qMax(textLine->length(), 0));
    }

    bool found = true;
    while (found) {
      uint foundAt;
      found = textLine->searchText(0, cursor.column(), text, &foundAt, 0, true, true);
      if (found) {
        bool hasStyle = true;
        if (attribute != -1) {
          KTextEditor::Attribute::Ptr a = attributes[textLine->attribute(foundAt)];
          const int ds = a->property(KateExtendedAttribute::AttributeDefaultStyleIndex).toInt();
          hasStyle = (ds == attribute);
        }

        if (hasStyle) {
          QScriptValue position = engine()->newObject();
          position.setProperty("line", QScriptValue(engine(), cursor.line()));
          position.setProperty("column", QScriptValue(engine(), foundAt));
          return position;
        } else {
          cursor.setColumn(foundAt);
        }
      }
    }
  } while (cursor.gotoPreviousLine());

  return QScriptValue();
}

KTextEditor::Cursor KateScriptDocument::anchor(int line, int column, QChar character)
{
  KateDocCursor cursor(line, column, m_document);
  QList<KTextEditor::Attribute::Ptr> attributes =
      m_document->highlight()->attributes(((KateView*) m_document->activeView())->renderer()->config()->schema());
  int count = 1;
  QChar lc = character;
  QChar rc;
  if (lc == '(') rc = ')';
  else if (lc == '{') rc = '}';
  else if (lc == '[') rc = ']';
  else return KTextEditor::Cursor::invalid ();

  // Move backwards char by char and find the opening character
  while (cursor.moveBackward(1)) {
    QChar ch = cursor.currentChar();
    if (ch == lc) {
      KTextEditor::Attribute::Ptr a = attributes[cursor.currentAttrib()];
      const int ds = a->property(KateExtendedAttribute::AttributeDefaultStyleIndex).toInt();
      if (_isCode(ds)) {
        --count;
      }
    }
    else if (ch == rc) {
      KTextEditor::Attribute::Ptr a = attributes[cursor.currentAttrib()];
      const int ds = a->property(KateExtendedAttribute::AttributeDefaultStyleIndex).toInt();
      if (_isCode(ds)) {
        ++count;
      }
    }

    if (count == 0) {
      return cursor;
    }
  }
  return KTextEditor::Cursor::invalid ();
}

bool KateScriptDocument::startsWith (int line, const QString &pattern, bool skipWhiteSpaces)
{
  KateTextLine::Ptr textLine = m_document->plainKateTextLine(line);

  if (!textLine)
    return false;

  if (skipWhiteSpaces)
    return textLine->matchesAt(textLine->firstChar(), pattern);

  return textLine->startsWith (pattern);
}

bool KateScriptDocument::endsWith (int line, const QString &pattern, bool skipWhiteSpaces)
{
  KateTextLine::Ptr textLine = m_document->plainKateTextLine(line);

  if (!textLine)
    return false;

  if (skipWhiteSpaces)
    return textLine->matchesAt(textLine->lastChar() - pattern.length() + 1, pattern);

  return textLine->endsWith (pattern);
}

//BEGIN Automatically generated

QString KateScriptDocument::fileName()
{
  return m_document->documentName();
}

QString KateScriptDocument::url()
{
  return m_document->url().prettyUrl();
}

QString KateScriptDocument::mimeType()
{
  return m_document->mimeType();
}

QString KateScriptDocument::encoding()
{
  return m_document->encoding();
}

bool KateScriptDocument::isModified()
{
  return m_document->isModified();
}

QString KateScriptDocument::text()
{
  return m_document->text();
}

QString KateScriptDocument::textRange(int i, int j, int k, int l)
{
  return m_document->text(KTextEditor::Range(i, j, k, l));
}

QString KateScriptDocument::line(int i)
{
  return m_document->line (i);
}

QString KateScriptDocument::wordAt(int i, int j)
{
  return m_document->getWord(KTextEditor::Cursor(i, j));
}

QString KateScriptDocument::charAt(int i, int j)
{
  const QChar c = m_document->character (KTextEditor::Cursor(i, j));
  return c.isNull() ? "" : QString(c);
}

QString KateScriptDocument::firstChar(int i)
{
  KateTextLine::Ptr textLine = m_document->plainKateTextLine(i);
  if(!textLine) return "";
  // check for isNull(), as the returned character then would be "\0"
  const QChar c = textLine->at(textLine->firstChar());
  return c.isNull() ? "" : QString(c);
}

QString KateScriptDocument::lastChar(int i)
{
  KateTextLine::Ptr textLine = m_document->plainKateTextLine(i);
  if(!textLine) return "";
  // check for isNull(), as the returned character then would be "\0"
  const QChar c = textLine->at(textLine->lastChar());
  return c.isNull() ? "" : QString(c);
}

bool KateScriptDocument::isSpace(int i, int j)
{
  return m_document->character (KTextEditor::Cursor(i, j)).isSpace();
}

bool KateScriptDocument::matchesAt(int i, int j, const QString &s)
{
  KateTextLine::Ptr textLine = m_document->plainKateTextLine(i);
  return textLine ? textLine->matchesAt(j, s) : false;
}

bool KateScriptDocument::setText(const QString &s)
{
  return m_document->setText(s);
}

bool KateScriptDocument::clear()
{
  return m_document->clear();
}

bool KateScriptDocument::truncate(int i, int j)
{
  KateTextLine::Ptr textLine = m_document->plainKateTextLine(i);
  if(textLine) textLine->truncate(j);
  return static_cast<bool>(textLine);
}

bool KateScriptDocument::insertText(int i, int j, const QString &s)
{
  return m_document->insertText (KTextEditor::Cursor(i, j), s);
}

bool KateScriptDocument::removeText(int i, int j, int k, int l)
{
  return m_document->removeText(KTextEditor::Range(i, j, k, l));
}

bool KateScriptDocument::insertLine(int i, const QString &s)
{
  return m_document->insertLine (i, s);
}

bool KateScriptDocument::removeLine(int i)
{
  return m_document->removeLine (i);
}

void KateScriptDocument::joinLines(int i, int j)
{
  m_document->joinLines (i, j);
  return;
}

int KateScriptDocument::lines()
{
  return m_document->lines();
}

int KateScriptDocument::length()
{
  return m_document->totalCharacters();
}

int KateScriptDocument::lineLength(int i)
{
  return m_document->lineLength(i);
}

void KateScriptDocument::editBegin()
{
  m_document->editBegin();
  return;
}

void KateScriptDocument::editEnd()
{
  m_document->editEnd ();
  return;
}

int KateScriptDocument::firstColumn(int i)
{
  KateTextLine::Ptr textLine = m_document->plainKateTextLine(i);
  return textLine ? textLine->firstChar() : -1;
}

int KateScriptDocument::lastColumn(int i)
{
  KateTextLine::Ptr textLine = m_document->plainKateTextLine(i);
  return textLine ? textLine->lastChar() : -1;
}

int KateScriptDocument::prevNonSpaceColumn(int i, int j)
{
  KateTextLine::Ptr textLine = m_document->plainKateTextLine(i);
  if(!textLine) return -1;
  return textLine->previousNonSpaceChar(j);
}

int KateScriptDocument::nextNonSpaceColumn(int i, int j)
{
  KateTextLine::Ptr textLine = m_document->plainKateTextLine(i);
  if(!textLine) return -1;
  return textLine->nextNonSpaceChar(j);
}

int KateScriptDocument::prevNonEmptyLine(int i)
{
  const int startLine = i;
  for (int currentLine = startLine; currentLine >= 0; --currentLine) {
    KateTextLine::Ptr textLine = m_document->plainKateTextLine(currentLine);
    if(!textLine)
      return -1;
    if(textLine->firstChar() != -1)
      return currentLine;
  }
  return -1;
}

int KateScriptDocument::nextNonEmptyLine(int i)
{
  const int startLine = i;
  for (int currentLine = startLine; currentLine < m_document->lines(); ++currentLine) {
    KateTextLine::Ptr textLine = m_document->plainKateTextLine(currentLine);
    if(!textLine)
      return -1;
    if(textLine->firstChar() != -1)
      return currentLine;
  }
  return -1;
}

bool KateScriptDocument::isInWord(const QString &s, int i)
{
  return m_document->highlight()->isInWord(s.at(0), i);
}

bool KateScriptDocument::canBreakAt(const QString &s, int i)
{
  return m_document->highlight()->canBreakAt(s.at(0), i);
}

bool KateScriptDocument::canComment(int i, int j)
{
  return m_document->highlight()->canComment(i, j);
}

QString KateScriptDocument::commentMarker(int i)
{
  return m_document->highlight()->getCommentSingleLineStart(i);
}

QString KateScriptDocument::commentStart(int i)
{
  return m_document->highlight()->getCommentStart(i);
}

QString KateScriptDocument::commentEnd(int i)
{
  return m_document->highlight()->getCommentEnd(i);
}

int KateScriptDocument::attribute(int line, int column)
{
  KateTextLine::Ptr textLine = m_document->kateTextLine(line);
  if(!textLine) return 0;
  return textLine->attribute(column);
}

bool KateScriptDocument::isAttribute(int line, int column, int attr)
{
  return attr == attribute(line, column);
}

QString KateScriptDocument::attributeName(int line, int column)
{
  KateDocCursor cursor(line, column, m_document);
  QList<KTextEditor::Attribute::Ptr> attributes = m_document->highlight()->attributes(((KateView*) m_document->activeView())->renderer()->config()->schema());
  KTextEditor::Attribute::Ptr a = attributes[cursor.currentAttrib()];
  return a->property(KateExtendedAttribute::AttributeName).toString();
}

bool KateScriptDocument::isAttributeName(int line, int column, const QString &name)
{
  return name == attributeName(line, column);
}

QString KateScriptDocument::variable(const QString &s)
{
  return m_document->variable(s);
}

//END

bool KateScriptDocument::_isCode(int defaultStyle)
{
  return (defaultStyle != KateExtendedAttribute::dsComment
       && defaultStyle != KateExtendedAttribute::dsString
       && defaultStyle != KateExtendedAttribute::dsRegionMarker
       && defaultStyle != KateExtendedAttribute::dsChar
       && defaultStyle != KateExtendedAttribute::dsOthers);
}

// kate: space-indent on; indent-width 2; replace-tabs on;

