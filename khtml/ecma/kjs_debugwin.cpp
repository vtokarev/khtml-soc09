/*
 *  This file is part of the KDE libraries
 *  Copyright (C) 2000-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2001,2003 Peter Kelly (pmk@post.com)
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "kjs_debugwin.h"
#include "kjs_proxy.h"

#ifdef KJS_DEBUGGER

#include <assert.h>
#include <stdlib.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <q3textedit.h>
#include <qlistwidget.h>
#include <q3multilineedit.h>
#include <qapplication.h>
#include <ktoolbar.h>
#include <qsplitter.h>
#include <qcombobox.h>
#include <qbitmap.h>
#include <qwidget.h>
#include <qlabel.h>
#include <qdatastream.h>
#include <qpainter.h>
#include <qscrollbar.h>
#include <qlist.h>

#include <klocale.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <kglobal.h>
#include <kmessagebox.h>
#include <kguiitem.h>
#include <kmenu.h>
#include <kmenubar.h>
#include <kaction.h>
#include <kactioncollection.h>
#include <kglobalsettings.h>
#include <kshortcut.h>
#include <kconfig.h>
#include <kstringhandler.h>

#include "kjs_dom.h"
#include "kjs_binding.h"
#include "khtml_part.h"
#include "khtmlview.h"
#include "khtml_pagecache.h"
#include "khtml_settings.h"
#include "khtml_factory.h"
#include <kencodingdetector.h>
#include <kjs/ustring.h>
#include <kjs/object.h>
#include <kjs/function.h>
#include <kjs/interpreter.h>
#include <kjs/value.h>

#define QT_NO_KEYWORDS
#include <QtDBus/QtDBus>

using namespace KJS;
using namespace khtml;

SourceDisplay::SourceDisplay(KJSDebugWin *debugWin, QWidget *parent, const char *name)
  : Q3ScrollView(parent,name), m_currentLine(-1), m_sourceFile(0), m_debugWin(debugWin),
    m_font(KGlobalSettings::fixedFont())
{
  verticalScrollBar()->setSingleStep(QFontMetrics(m_font).height());
  viewport()->setAttribute(Qt::WA_NoSystemBackground);
  m_breakpointIcon = KIconLoader::global()->loadIcon("process-stop",KIconLoader::Small);
}

SourceDisplay::~SourceDisplay()
{
  if (m_sourceFile) {
    m_sourceFile->deref();
    m_sourceFile = 0L;
  }
}

void SourceDisplay::setSource(SourceFile *sourceFile)
{
  if ( sourceFile )
      sourceFile->ref();
  if (m_sourceFile)
      m_sourceFile->deref();
  m_sourceFile = sourceFile;
  if ( m_sourceFile )
      m_sourceFile->ref();

  if (!m_sourceFile || !m_debugWin->isVisible()) {
    return;
  }

  QString code = sourceFile->getCode();
  const QChar *chars = code.unicode();
  uint len = code.length();
  QChar newLine('\n');
  QChar cr('\r');
  QChar tab('\t');
  QString tabstr("        ");
  QString line;
  m_lines.clear();
  int width = 0;
  QFontMetrics metrics(m_font);

  for (uint pos = 0; pos < len; pos++) {
    QChar c = chars[pos];
    if (c == cr) {
      if (pos < len-1 && chars[pos+1] == newLine)
	continue;
      else
	c = newLine;
    }
    if (c == newLine) {
      m_lines.append(line);
      int lineWidth = metrics.width(line);
      if (lineWidth > width)
	width = lineWidth;
      line = "";
    }
    else if (c == tab) {
      line += tabstr;
    }
    else {
      line += c;
    }
  }
  if (line.length()) {
    m_lines.append(line);
    int lineWidth = metrics.width(line);
            m_documents[key]->setFullSource(
    if (lineWidth > width)
      width = lineWidth;
  }

  int linenoDisplayWidth = metrics.width("888888");
  resizeContents(linenoDisplayWidth+4+width,metrics.height()*m_lines.count());
  update();
  sourceFile->deref();
}

void SourceDisplay::setCurrentLine(int lineno, bool doCenter)
{
  m_currentLine = lineno;

  if (doCenter && m_currentLine >= 0) {
    QFontMetrics metrics(m_font);
    int height = metrics.height();
    center(0,height*m_currentLine+height/2);
  }

  updateContents();
}

void SourceDisplay::contentsMousePressEvent(QMouseEvent *e)
{
  Q3ScrollView::mouseDoubleClickEvent(e);
  QFontMetrics metrics(m_font);
  int lineno = e->y()/metrics.height();
  emit lineDoubleClicked(lineno+1); // line numbers start from 1
}

void SourceDisplay::showEvent(QShowEvent *)
{
    setSource(m_sourceFile);
}

void SourceDisplay::drawContents(QPainter *p, int clipx, int clipy, int clipw, int cliph)
{
  if (!m_sourceFile) {
    p->fillRect(clipx,clipy,clipw,cliph,palette().color( QPalette::Active, QPalette::Base ));
    return;
  }

  QFontMetrics metrics(m_font);
  int height = metrics.height();

  int bottom = clipy + cliph;
  int right = clipx + clipw;

  int firstLine = clipy/height-1;
  if (firstLine < 0)
    firstLine = 0;
  int lastLine = bottom/height+2;
  if (lastLine > (int)m_lines.count())
    lastLine = m_lines.count();

  p->setFont(m_font);

  int linenoWidth = metrics.width("888888");

  for (int lineno = firstLine; lineno <= lastLine; lineno++) {
    QString linenoStr = QString().sprintf("%d",lineno+1);


    p->fillRect(0,height*lineno,linenoWidth,height,palette().color( QPalette::Active, QPalette::Mid ));

    p->setPen(palette().color( QPalette::Active, QPalette::Text ));
    p->drawText(0,height*lineno,linenoWidth,height,Qt::AlignRight,linenoStr);

    QColor bgColor;
    QColor textColor;

    if (lineno == m_currentLine) {
      bgColor = palette().color( QPalette::Active, QPalette::Highlight );
      textColor = palette().color( QPalette::Active, QPalette::HighlightedText );
    }
    else if (m_debugWin->haveBreakpoint(m_sourceFile,lineno+1,lineno+1)) {
      bgColor = palette().color( QPalette::Active, QPalette::Text );
      textColor = palette().color( QPalette::Active, QPalette::Base );
      p->drawPixmap(2,height*lineno+height/2-m_breakpointIcon.height()/2,m_breakpointIcon);
    }
    else {
      bgColor = palette().color( QPalette::Active, QPalette::Base );
      textColor = palette().color( QPalette::Active, QPalette::Text );
    }

    p->fillRect(linenoWidth,height*lineno,right-linenoWidth,height,bgColor);
    p->setPen(textColor);
    p->drawText(linenoWidth+4,height*lineno,contentsWidth()-linenoWidth-4,height,
		Qt::AlignLeft,m_lines[lineno]);
  }

  int remainingTop = height*(lastLine+1);
  p->fillRect(0,remainingTop,linenoWidth,bottom-remainingTop,palette().color( QPalette::Active, QPalette::Mid ));

  p->fillRect(linenoWidth,remainingTop,
	      right-linenoWidth,bottom-remainingTop,palette().color( QPalette::Active, QPalette::Base ));
}

//-------------------------------------------------------------------------

KJSDebugWin * KJSDebugWin::kjs_html_debugger = 0;

QString SourceFile::getCode()
{
  if (interpreter) {
    KHTMLPart *part = qobject_cast<KHTMLPart*>(static_cast<ScriptInterpreter*>(interpreter)->part());
    if (part && url == part->url().url() && KHTMLPageCache::self()->isValid(part->cacheId())) {
      KEncodingDetector *decoder = part->createDecoder();
      QByteArray data;
      QDataStream stream(&data,QIODevice::WriteOnly);
      KHTMLPageCache::self()->saveData(part->cacheId(),&stream);
      QString str;
      if (data.size() == 0)
        str = "";
      else
      {
        str = decoder->decode(data);
      }
      delete decoder;
      return str;
    }
  }

  return code;
}

//-------------------------------------------------------------------------

SourceFragment::SourceFragment(int sid, int bl, int el, SourceFile *sf)
{
  sourceId = sid;
  baseLine = bl;
  errorLine = el;
  sourceFile = sf;
  sourceFile->ref();
}

SourceFragment::~SourceFragment()
{
  sourceFile->deref();
  sourceFile = 0L;
}

//-------------------------------------------------------------------------

KJSErrorDialog::KJSErrorDialog(QWidget *parent, const QString& errorMessage, bool showDebug)
  : KDialog( parent )
{
  setCaption( i18n("JavaScript Error") );
  setModal( true );
  setButtons( showDebug ? KDialog::Ok | KDialog::User1 : KDialog::Ok );
  setButtonGuiItem( KDialog::User1, KGuiItem("&Debug","system-run") );
  setDefaultButton( KDialog::Ok );
  showButtonSeparator( false );

  QWidget *page = new QWidget(this);
  setMainWidget(page);

  QLabel *iconLabel = new QLabel("",page);
  iconLabel->setPixmap(KIconLoader::global()->loadIcon("dialog-error",
                                                    KIconLoader::NoGroup,KIconLoader::SizeMedium,
                                                    KIconLoader::DefaultState,0,true));

  QWidget *contents = new QWidget(page);
  QLabel *label = new QLabel(errorMessage,contents);
  m_dontShowAgainCb = new QCheckBox(i18n("&Do not show this message again"),contents);

  QVBoxLayout *vl = new QVBoxLayout(contents);
  vl->setMargin(0);
  vl->setSpacing(spacingHint());
  vl->addWidget(label);
  vl->addWidget(m_dontShowAgainCb);

  QHBoxLayout *topLayout = new QHBoxLayout(page);
  topLayout->setMargin(0);
  topLayout->setSpacing(spacingHint());
  topLayout->addWidget(iconLabel);
  topLayout->addWidget(contents);
  topLayout->addStretch(10);

  m_debugSelected = false;
  connect(this,SIGNAL(user1Clicked()),this,SLOT(slotUser1()));
}

KJSErrorDialog::~KJSErrorDialog()
{
}

void KJSErrorDialog::slotUser1()
{
  m_debugSelected = true;
  close();
}

//-------------------------------------------------------------------------
EvalMultiLineEdit::EvalMultiLineEdit(QWidget *parent)
    : Q3MultiLineEdit(parent) {
}

void EvalMultiLineEdit::keyPressEvent(QKeyEvent * e)
{
    if (e->key() == Qt::Key_Return) {
        if (hasSelectedText()) {
            m_code = selectedText();
        } else {
            int para, index;
            getCursorPosition(&para, &index);
            m_code = text(para);
        }
        end();
    }
    Q3MultiLineEdit::keyPressEvent(e);
}
//-------------------------------------------------------------------------
KJSDebugWin::KJSDebugWin(QWidget *parent, const char *name)
  : KMainWindow(parent, name, Qt::WType_TopLevel), KComponentData("kjs_debugger")
{
  m_breakpoints = 0;
  m_breakpointCount = 0;

  m_curSourceFile = 0;
  m_mode = Continue;
  m_nextSourceUrl = "";
  m_nextSourceBaseLine = 1;
  m_execs = 0;
  m_execsCount = 0;
  m_execsAlloc = 0;
  m_steppingDepth = 0;

  m_stopIcon = KIconLoader::global()->loadIcon("process-stop",KIconLoader::Small);
  m_emptyIcon = QPixmap(m_stopIcon.width(),m_stopIcon.height());
  QBitmap emptyMask(m_stopIcon.width(),m_stopIcon.height());
  emptyMask.clear();
  m_emptyIcon.setMask(emptyMask);

  setCaption(i18n("JavaScript Debugger"));

  QWidget *mainWidget = new QWidget(this);
  setCentralWidget(mainWidget);

  QVBoxLayout *vl = new QVBoxLayout(mainWidget);
  vl->setMargin(5);

  // frame list & code
  QSplitter *hsplitter = new QSplitter(Qt::Vertical,mainWidget);
  QSplitter *vsplitter = new QSplitter(hsplitter);
  QFont font(KGlobalSettings::fixedFont());

  QWidget *contextContainer = new QWidget(vsplitter);

  QLabel *contextLabel = new QLabel(i18n("Call stack"),contextContainer);
  QWidget *contextListContainer = new QWidget(contextContainer);
  m_contextList = new QListWidget(contextListContainer);
  m_contextList->setMinimumSize(100,200);
  connect(m_contextList,SIGNAL(currentRowChanged(int)),this,SLOT(slotShowFrame(int)));

  QHBoxLayout *clistLayout = new QHBoxLayout(contextListContainer);
  clistLayout->addWidget(m_contextList);
  clistLayout->addSpacing(KDialog::spacingHint());

  QVBoxLayout *contextLayout = new QVBoxLayout(contextContainer);
  contextLayout->addWidget(contextLabel);
  contextLayout->addSpacing(KDialog::spacingHint());
  contextLayout->addWidget(contextListContainer);

  // source selection & display
  QWidget *sourceSelDisplay = new QWidget(vsplitter);
  QVBoxLayout *ssdvl = new QVBoxLayout(sourceSelDisplay);

  m_sourceSel = new QComboBox(toolBar());
  connect(m_sourceSel,SIGNAL(activated(int)),this,SLOT(slotSourceSelected(int)));

  m_sourceDisplay = new SourceDisplay(this,sourceSelDisplay);
  ssdvl->addWidget(m_sourceDisplay);
  connect(m_sourceDisplay,SIGNAL(lineDoubleClicked(int)),SLOT(slotToggleBreakpoint(int)));

  QList<int> vsplitSizes;
  vsplitSizes.insert(vsplitSizes.end(),120);
  vsplitSizes.insert(vsplitSizes.end(),480);
  vsplitter->setSizes(vsplitSizes);

  // evaluate

  QWidget *evalContainer = new QWidget(hsplitter);

  QLabel *evalLabel = new QLabel(i18n("JavaScript console"),evalContainer);
  m_evalEdit = new EvalMultiLineEdit(evalContainer);
  m_evalEdit->setWordWrap(Q3MultiLineEdit::NoWrap);
  m_evalEdit->setFont(font);
  connect(m_evalEdit,SIGNAL(returnPressed()),SLOT(slotEval()));
  m_evalDepth = 0;

  QVBoxLayout *evalLayout = new QVBoxLayout(evalContainer);
  evalLayout->addSpacing(KDialog::spacingHint());
  evalLayout->addWidget(evalLabel);
  evalLayout->addSpacing(KDialog::spacingHint());
  evalLayout->addWidget(m_evalEdit);

  QList<int> hsplitSizes;
  hsplitSizes.insert(hsplitSizes.end(),400);
  hsplitSizes.insert(hsplitSizes.end(),200);
  hsplitter->setSizes(hsplitSizes);

  vl->addWidget(hsplitter);

  // actions
  KMenu *debugMenu = new KMenu(this);
  menuBar()->insertItem("&Debug",debugMenu);

  m_actionCollection = new KActionCollection(this);
  m_actionCollection->setComponentData(*this);

  // Venkman use F12, KDevelop F10
  KShortcut scNext = KShortcut(KKeySequence(KKey(Qt::Key_F12)));
  scNext.append(KKeySequence(KKey(Qt::Key_F10)));
  m_nextAction       = new KAction(i18nc("Next breakpoint","&Next"),"dbgnext",scNext,this,SLOT(slotNext()),
				   m_actionCollection,"next");
  m_stepAction       = new KAction(i18n("&Step"),"dbgstep",KShortcut(Qt::Key_F11),this,SLOT(slotStep()),
				   m_actionCollection,"step");
  // Venkman use F5, Kdevelop F9
  KShortcut scCont = KShortcut(KKeySequence(KKey(Qt::Key_F5)));
  scCont.append(KKeySequence(KKey(Qt::Key_F9)));
  m_continueAction   = new KAction(i18n("&Continue"),"dbgrun",scCont,this,SLOT(slotContinue()),
				   m_actionCollection,"cont");
  m_stopAction       = new KAction(i18n("St&op"),"stop",KShortcut(Qt::Key_F4),this,SLOT(slotStop()),
				   m_actionCollection,"stop");
  m_breakAction      = new KAction(i18n("&Break at Next Statement"),"dbgrunto",KShortcut(Qt::Key_F8),this,SLOT(slotBreakNext()),
				   m_actionCollection,"breaknext");

  m_nextAction->setToolTip(i18nc("Next breakpoint","Next"));
  m_stepAction->setToolTip(i18n("Step"));
  m_continueAction->setToolTip(i18n("Continue"));
  m_stopAction->setToolTip(i18n("Stop"));
  m_breakAction->setToolTip("Break at next Statement");

  m_nextAction->setEnabled(false);
  m_stepAction->setEnabled(false);
  m_continueAction->setEnabled(false);
  m_stopAction->setEnabled(false);
  m_breakAction->setEnabled(true);

  debugMenu->addAction( m_nextAction );
  debugMenu->addAction( m_stepAction );
  debugMenu->addAction( m_continueAction );
//   m_stopAction->plug(debugMenu); ### disabled until DebuggerImp::stop() works reliably
  debugMenu->addAction( m_breakAction );

  toolBar()->addAction( m_nextAction );
  toolBar()->addAction( m_stepAction );
  toolBar()->addAction( m_continueAction );
//   m_stopAction->plug(toolBar()); ###
  toolBar()->addAction( m_breakAction );

  toolBar()->addWidget(m_sourceSel);

  updateContextList();
  setMinimumSize(300,200);
  resize(600,450);

}

KJSDebugWin::~KJSDebugWin()
{
  free(m_breakpoints);
  free(m_execs);
}

KJSDebugWin *KJSDebugWin::createInstance()
{
  assert(!kjs_html_debugger);
  kjs_html_debugger = new KJSDebugWin();
  return kjs_html_debugger;
}

void KJSDebugWin::destroyInstance()
{
  assert(kjs_html_debugger);
  kjs_html_debugger->hide();
  delete kjs_html_debugger;
}

void KJSDebugWin::slotNext()
{
  m_mode = Next;
  leaveSession();
}

void KJSDebugWin::slotStep()
{
  m_mode = Step;
  leaveSession();
}

void KJSDebugWin::slotContinue()
{
  m_mode = Continue;
  leaveSession();
}

void KJSDebugWin::slotStop()
{
  m_mode = Stop;
  while (!m_execStates.isEmpty())
    leaveSession();
}

void KJSDebugWin::slotBreakNext()
{
  m_mode = Step;
}

void KJSDebugWin::slotToggleBreakpoint(int lineno)
{
  if (m_sourceSel->currentIndex() < 0)
    return;

  SourceFile *sourceFile = m_sourceSelFiles.at(m_sourceSel->currentIndex());

  // Find the source fragment containing the selected line (if any)
  int sourceId = -1;
  int highestBaseLine = -1;
  QMap<int,SourceFragment*>::Iterator it;

  for (it = m_sourceFragments.begin(); it != m_sourceFragments.end(); ++it) {
    SourceFragment *sourceFragment = it.value();
    if (sourceFragment &&
	sourceFragment->sourceFile == sourceFile &&
	sourceFragment->baseLine <= lineno &&
	sourceFragment->baseLine > highestBaseLine) {

	sourceId = sourceFragment->sourceId;
	highestBaseLine = sourceFragment->baseLine;
    }
  }

  if (sourceId < 0)
    return;

  // Update the source code display with the appropriate icon
  int fragmentLineno = lineno-highestBaseLine+1;
  if (!setBreakpoint(sourceId,fragmentLineno)) // was already set
    deleteBreakpoint(sourceId,fragmentLineno);

  m_sourceDisplay->updateContents();
}

void KJSDebugWin::slotShowFrame(int frameno)
{
  if (frameno < 0 || frameno >= m_execsCount)
    return;

  Context ctx = m_execs[frameno]->context();
  abort();
  //setSourceLine(ctx.sourceId(),ctx.curStmtFirstLine());
}

void KJSDebugWin::slotSourceSelected(int sourceSelIndex)
{
  // A source file has been selected from the drop-down list - display the file
  if (sourceSelIndex < 0 || sourceSelIndex >= (int)m_sourceSel->count())
    return;
  SourceFile *sourceFile = m_sourceSelFiles.at(sourceSelIndex);
  displaySourceFile(sourceFile,true);

  // If the currently selected context is in the current source file, then hilight
  // the line it's on.
  if (m_contextList->currentRow() >= 0) {
    Context ctx = m_execs[m_contextList->currentRow()]->context();
    abort();
    //if (m_sourceFragments[ctx.sourceId()]->sourceFile == m_sourceSelFiles.at(sourceSelIndex))
    //  setSourceLine(ctx.sourceId(),ctx.curStmtFirstLine());
  }
}

void KJSDebugWin::slotEval()
{
  // Work out which execution state to use. If we're currently in a debugging session,
  // use the current context - otherwise, use the global execution state from the interpreter
  // corresponding to the currently displayed source file.
  ExecState *exec;
  JSObject *thisobj;
  if (m_execStates.isEmpty()) {
    if (m_sourceSel->currentIndex() < 0)
      return;
    SourceFile *sourceFile = m_sourceSelFiles.at(m_sourceSel->currentIndex());
    if (!sourceFile->interpreter)
      return;
    exec = sourceFile->interpreter->globalExec();
    thisobj = exec->interpreter()->globalObject();
  }
  else {
    exec = m_execStates.top();
    thisobj = exec->context().thisValue();
  }

  // Evaluate the js code from m_evalEdit
  UString code(m_evalEdit->code());
  QString msg;

  KJSCPUGuard guard;
  guard.start();

  Interpreter *interp = exec->interpreter();

  JSObject *obj = interp->globalObject()->get(exec, "eval")->getObject();
  List args;
  args.append(String(code));

  m_evalDepth++;
  JSValue *retval = obj->call(exec, thisobj, args);
  m_evalDepth--;
  guard.stop();

  // Print the return value or exception message to the console
  if (exec->hadException()) {
    JSValue *exc = exec->exception();
    exec->clearException();
    msg = "Exception: " + exc->toString(interp->globalExec()).qstring();
  }
  else {
    msg = retval->toString(interp->globalExec()).qstring();
  }

  m_evalEdit->insert(msg+'\n');
  updateContextList();
}

void KJSDebugWin::closeEvent(QCloseEvent *e)
{
  while (!m_execStates.isEmpty()) // ### not sure if this will work
    leaveSession();
  return QWidget::closeEvent(e);
}

bool KJSDebugWin::eventFilter(QObject *o, QEvent *e)
{
  switch (e->type()) {
  case QEvent::MouseButtonPress:
  case QEvent::MouseButtonRelease:
  case QEvent::MouseButtonDblClick:
  case QEvent::MouseMove:
  case QEvent::KeyPress:
  case QEvent::KeyRelease:
  case QEvent::Destroy:
  case QEvent::Close:
  case QEvent::Quit:
    while (o->parent())
      o = o->parent();
    if (o == this)
      return QWidget::eventFilter(o,e);
    else
      return true;
    break;
  default:
    return QWidget::eventFilter(o,e);
  }
}

void KJSDebugWin::disableOtherWindows()
{
  QWidgetList widgets = QApplication::allWidgets();
  QListIterator<QWidget*> it(widgets);
  while (it.hasNext()) {
    QWidget* widget = it.next();
    widget->installEventFilter(this);
  }
}

void KJSDebugWin::enableOtherWindows()
{
  QWidgetList widgets = QApplication::allWidgets();
  QListIterator<QWidget*> it(widgets);
  while (it.hasNext()) {
    QWidget* widget = it.next();
    widget->removeEventFilter(this);
  }
}

bool KJSDebugWin::sourceParsed(KJS::ExecState *exec, int sourceId,
                               const KJS::UString &source, int errorLine)
{
  // Work out which source file this fragment is in
  SourceFile *sourceFile = 0;
  if (!m_nextSourceUrl.isEmpty())
    sourceFile = getSourceFile(exec->interpreter(),m_nextSourceUrl);

  int index;
  if (!sourceFile) {
    index = m_sourceSel->count();
    if (!m_nextSourceUrl.isEmpty()) {

      QString code = source.qstring();
      KParts::ReadOnlyPart *part = static_cast<ScriptInterpreter*>(exec->interpreter())->part();
      if (m_nextSourceUrl == part->url().url()) {
	// Only store the code here if it's not from the part's html page... in that
	// case we can get it from KHTMLPageCache
	code.clear();
      }

      sourceFile = new SourceFile(m_nextSourceUrl,code,exec->interpreter());
      setSourceFile(exec->interpreter(),m_nextSourceUrl,sourceFile);
      m_sourceSelFiles.append(sourceFile);
      m_sourceSel->addItem(m_nextSourceUrl);
    }
    else {
      // Sourced passed from somewhere else (possibly an eval call)... we don't know the url,
      // but we still know the interpreter
      sourceFile = new SourceFile("(unknown)",source.qstring(),exec->interpreter());
      m_sourceSelFiles.append(sourceFile);
      m_sourceSel->addItem(QString::number(index) += "-???");
    }
  }
  else {
    // Ensure that each source file to be displayed is associated with
    // an appropriate interpreter
    if (!sourceFile->interpreter)
      sourceFile->interpreter = exec->interpreter();
    for (index = 0; index < m_sourceSel->count(); index++) {
      if (m_sourceSelFiles.at(index) == sourceFile)
	break;
    }
    assert(index < m_sourceSel->count());
  }

  SourceFragment *sf = new SourceFragment(sourceId,m_nextSourceBaseLine,errorLine,sourceFile);
  m_sourceFragments[sourceId] = sf;

  if (m_sourceSel->currentIndex() < 0)
    m_sourceSel->setCurrentIndex(index);

  if (m_sourceSel->currentIndex() == index) {
    displaySourceFile(sourceFile,true);
  }

  m_nextSourceBaseLine = 1;
  m_nextSourceUrl = "";

  return (m_mode != Stop);
}

bool KJSDebugWin::sourceUnused(KJS::ExecState *exec, int sourceId)
{
  // Verify that there aren't any contexts on the stack using the given sourceId
  // This should never be the case because this function is only called when
  // the interpreter has deleted all Node objects for the source.
  for (int e = 0; e < m_execsCount; e++)
    abort();
    //assert(m_execs[e]->context().sourceId() != sourceId);

  // Now remove the fragment (and the SourceFile, if it was the last fragment in that file)
  SourceFragment *fragment = m_sourceFragments[sourceId];
  if (fragment) {
    m_sourceFragments.remove(sourceId);

    SourceFile *sourceFile = fragment->sourceFile;
    if (sourceFile->hasOneRef()) {
      for (int i = 0; i < m_sourceSel->count(); i++) {
	if (m_sourceSelFiles.at(i) == sourceFile) {
	  m_sourceSel->removeItem(i);
	  m_sourceSelFiles.remove(i);
	  break;
	}
      }
      removeSourceFile(exec->interpreter(),sourceFile->url);
    }
    delete fragment;
  }

  return (m_mode != Stop);
}

bool KJSDebugWin::exception(ExecState *exec, JSValue *value, bool inTryCatch)
{
  assert(value);

  // Ignore exceptions that will be caught by the script
  if (inTryCatch)
    return true;

  KParts::ReadOnlyPart *part = static_cast<ScriptInterpreter*>(exec->interpreter())->part();
  KHTMLPart *khtmlpart = qobject_cast<KHTMLPart*>(part);
  if (khtmlpart && !khtmlpart->settings()->isJavaScriptErrorReportingEnabled())
    return true;

  QWidget *dlgParent = (m_evalDepth == 0) ? (QWidget*)part->widget() : (QWidget*)this;

  QString exceptionMsg = value->toString(exec).qstring();

  // Syntax errors are a special case. For these we want to display the url & lineno,
  // which isn't included in the exception messeage. So we work it out from the values
  // passed to sourceParsed()
  JSObject *valueObj = value->getObject();;
  JSObject *syntaxError = exec->interpreter()->builtinSyntaxError();
  if (valueObj && valueObj->get(exec,"constructor") == syntaxError) {
    JSValue *sidValue = valueObj->get(exec,"sid");
    if (sidValue->type() == NumberType) { // sid is not set for Function() constructor
      int sourceId = (int)sidValue->toNumber(exec);
      assert(m_sourceFragments[sourceId]);
      exceptionMsg = i18n("Parse error at %1 line %2",
		      m_sourceFragments[sourceId]->sourceFile->url,
		      m_sourceFragments[sourceId]->baseLine+m_sourceFragments[sourceId]->errorLine-1);
    }
  }

  bool dontShowAgain = false;
  if (m_execsCount == 0) {
    // An exception occurred and we're not currently executing any code... this can
    // happen in some cases e.g. a parse error, or native code accessing funcitons like
    // Object::put()
    QString msg = i18n("An error occurred while attempting to run a script on this page.\n\n%1",
		   exceptionMsg);
    KJSErrorDialog dlg(dlgParent,msg,false);
    dlg.exec();
    dontShowAgain = dlg.dontShowAgain();
  }
  else {
    Context ctx = m_execs[m_execsCount-1]->context();
    abort();
#if 0
    SourceFragment *sourceFragment = m_sourceFragments[ctx.sourceId()];
    QString msg = i18n("An error occurred while attempting to run a script on this page.\n\n%1 line %2:\n%3",
 		       KStringHandler::rsqueeze( sourceFragment->sourceFile->url,80),
 		       sourceFragment->baseLine+ctx.curStmtFirstLine()-1,
		       exceptionMsg);

    KJSErrorDialog dlg(dlgParent,msg,true);
    dlg.exec();
    dontShowAgain = dlg.dontShowAgain();

    if (dlg.debugSelected()) {
      m_mode = Next;
      m_steppingDepth = m_execsCount-1;
      enterSession(exec);
    }
#endif
  }

  if (dontShowAgain) {
    KConfigGroup cg(KGlobal::config(), QLatin1String("Java/JavaScript Settings"));
    cg.writeEntry("ReportJavaScriptErrors",false);
    cg.sync();
#if 0
    QByteArray data;
    KApplication::dcopClient()->send( "konqueror*", "KonquerorIface", "reparseConfiguration()", data );
#elif defined __GNUC__
#warning KJS should have an interface
#endif
  }

  return (m_mode != Stop);
}

bool KJSDebugWin::atStatement(KJS::ExecState *exec)
{
  assert(m_execsCount > 0);
  assert(m_execs[m_execsCount-1] == exec);
  checkBreak(exec);
  return (m_mode != Stop);
}

bool KJSDebugWin::enterContext(ExecState *exec)
{
  if (m_execsCount >= m_execsAlloc) {
    m_execsAlloc += 10;
    m_execs = (ExecState**)realloc(m_execs,m_execsAlloc*sizeof(ExecState*));
  }
  m_execs[m_execsCount++] = exec;

  if (m_mode == Step)
    m_steppingDepth = m_execsCount-1;

  checkBreak(exec);
  return (m_mode != Stop);
}

bool KJSDebugWin::exitContext(ExecState *exec, const Completion &/*completion*/)
{
  assert(m_execsCount > 0);
  assert(m_execs[m_execsCount-1] == exec);

  checkBreak(exec);

  m_execsCount--;
  if (m_steppingDepth > m_execsCount-1)
    m_steppingDepth = m_execsCount-1;
  if (m_execsCount == 0)
    updateContextList();

  return (m_mode != Stop);
}

void KJSDebugWin::displaySourceFile(SourceFile *sourceFile, bool forceRefresh)
{
  if (m_curSourceFile == sourceFile && !forceRefresh)
    return;
  sourceFile->ref();
  m_sourceDisplay->setSource(sourceFile);
  if (m_curSourceFile)
     m_curSourceFile->deref();
  m_curSourceFile = sourceFile;
}

void KJSDebugWin::setSourceLine(int sourceId, int lineno)
{
  SourceFragment *source = m_sourceFragments[sourceId];
  if (!source)
    return;

  SourceFile *sourceFile = source->sourceFile;
  if (m_curSourceFile != source->sourceFile) {
      for (int i = 0; i < m_sourceSel->count(); i++)
	if (m_sourceSelFiles.at(i) == sourceFile)
	  m_sourceSel->setCurrentIndex(i);
      displaySourceFile(sourceFile,false);
  }
  m_sourceDisplay->setCurrentLine(source->baseLine+lineno-2);
}

void KJSDebugWin::setNextSourceInfo(QString url, int baseLine)
{
  m_nextSourceUrl = url;
  m_nextSourceBaseLine = baseLine;
}

void KJSDebugWin::sourceChanged(Interpreter *interpreter, QString url)
{
  SourceFile *sourceFile = getSourceFile(interpreter,url);
  if (sourceFile && m_curSourceFile == sourceFile)
    displaySourceFile(sourceFile,true);
}

void KJSDebugWin::clearInterpreter(Interpreter *interpreter)
{
  QMap<int,SourceFragment*>::Iterator it;

  for (it = m_sourceFragments.begin(); it != m_sourceFragments.end(); ++it)
    if (it.value() && it.value()->sourceFile->interpreter == interpreter)
      it.value()->sourceFile->interpreter = 0;
}

SourceFile *KJSDebugWin::getSourceFile(Interpreter *interpreter, QString url)
{
  QString key = QString("%1|%2").arg((long)interpreter).arg(url);
  return m_sourceFiles[key];
}

void KJSDebugWin::setSourceFile(Interpreter *interpreter, QString url, SourceFile *sourceFile)
{
  QString key = QString("%1|%2").arg((long)interpreter).arg(url);
  sourceFile->ref();
  if (SourceFile* oldFile = m_sourceFiles[key])
    oldFile->deref();
  m_sourceFiles[key] = sourceFile;
}

void KJSDebugWin::removeSourceFile(Interpreter *interpreter, QString url)
{
  QString key = QString("%1|%2").arg((long)interpreter).arg(url);
  if (SourceFile* oldFile = m_sourceFiles[key])
    oldFile->deref();
  m_sourceFiles.remove(key);
}

void KJSDebugWin::checkBreak(ExecState *exec)
{
  if (m_breakpointCount > 0) {
    Context ctx = m_execs[m_execsCount-1]->context();
    abort();
    //if (haveBreakpoint(ctx.sourceId(),ctx.curStmtFirstLine(),ctx.curStmtLastLine())) {
    //  m_mode = Next;
    //  m_steppingDepth = m_execsCount-1;
   // }
  }

  if ((m_mode == Step || m_mode == Next) && m_steppingDepth == m_execsCount-1)
    enterSession(exec);
}

void KJSDebugWin::enterSession(ExecState *exec)
{
  // This "enters" a new debugging session, i.e. enables usage of the debugging window
  // It re-enters the qt event loop here, allowing execution of other parts of the
  // program to continue while the script is stopped. We have to be a bit careful here,
  // i.e. make sure the user can't quit the app, and disable other event handlers which
  // could interfere with the debugging session.
  if (!isVisible())
    show();

  m_mode = Continue;

  if (m_execStates.isEmpty()) {
    disableOtherWindows();
    m_nextAction->setEnabled(true);
    m_stepAction->setEnabled(true);
    m_continueAction->setEnabled(true);
    m_stopAction->setEnabled(true);
    m_breakAction->setEnabled(false);
  }
  m_execStates.push(exec);

  updateContextList();

  qApp->enter_loop(); // won't return until leaveSession() is called
}

void KJSDebugWin::leaveSession()
{
  // Disables debugging for this window and returns to execute the rest of the script
  // (or aborts execution, if the user pressed stop). When this returns, the program
  // will exit the qt event loop, i.e. return to whatever processing was being done
  // before the debugger was stopped.
  assert(!m_execStates.isEmpty());

  m_execStates.pop();

  if (m_execStates.isEmpty()) {
    m_nextAction->setEnabled(false);
    m_stepAction->setEnabled(false);
    m_continueAction->setEnabled(false);
    m_stopAction->setEnabled(false);
    m_breakAction->setEnabled(true);
    m_sourceDisplay->setCurrentLine(-1);
    enableOtherWindows();
  }

  qApp->exit_loop();
}

void KJSDebugWin::updateContextList()
{
  disconnect(m_contextList,SIGNAL(currentRowChanged(int)),this,SLOT(slotShowFrame(int)));

  m_contextList->clear();
  for (int i = 0; i < m_execsCount; i++)
    m_contextList->addItem(contextStr(m_execs[i]->context()));

  if (m_execsCount > 0) {
    m_contextList->setCurrentRow(m_execsCount-1);
    Context ctx = m_execs[m_execsCount-1]->context();
    abort();
    //setSourceLine(ctx.sourceId(),ctx.curStmtFirstLine());
  }

  connect(m_contextList,SIGNAL(currentRowChanged(int)),this,SLOT(slotShowFrame(int)));
}

QString KJSDebugWin::contextStr(const Context &ctx)
{
  abort();
  QString str = "";
#if 0
  SourceFragment *sourceFragment = m_sourceFragments[ctx.sourceId()];
  QString url = sourceFragment->sourceFile->url;
  int fileLineno = sourceFragment->baseLine+ctx.curStmtFirstLine()-1;

  switch (ctx.codeType()) {
  case GlobalCode:
    str = QString("Global code at %1:%2").arg(url).arg(fileLineno);
    break;
  case EvalCode:
    str = QString("Eval code at %1:%2").arg(url).arg(fileLineno);
    break;
  case FunctionCode:
    if (!ctx.functionName().isNull())
      str = QString("%1() at %2:%3").arg(ctx.functionName().qstring()).arg(url).arg(fileLineno);
    else
      str = QString("Anonymous function at %1:%2").arg(url).arg(fileLineno);
    break;
  }
#endif
  return str;
}

bool KJSDebugWin::setBreakpoint(int sourceId, int lineno)
{
  if (haveBreakpoint(sourceId,lineno,lineno))
    return false;

  m_breakpointCount++;
  m_breakpoints = static_cast<Breakpoint*>(realloc(m_breakpoints,
						   m_breakpointCount*sizeof(Breakpoint)));
  m_breakpoints[m_breakpointCount-1].sourceId = sourceId;
  m_breakpoints[m_breakpointCount-1].lineno = lineno;

  return true;
}

bool KJSDebugWin::deleteBreakpoint(int sourceId, int lineno)
{
  for (int i = 0; i < m_breakpointCount; i++) {
    if (m_breakpoints[i].sourceId == sourceId && m_breakpoints[i].lineno == lineno) {

      memmove(m_breakpoints+i,m_breakpoints+i+1,(m_breakpointCount-i-1)*sizeof(Breakpoint));
      m_breakpointCount--;
      m_breakpoints = static_cast<Breakpoint*>(realloc(m_breakpoints, _breakpointCount*sizeof(Breakpoint)));
      return true;
    }
  }

  return false;
}

bool KJSDebugWin::haveBreakpoint(SourceFile *sourceFile, int line0, int line1)
{
  for (int i = 0; i < m_breakpointCount; i++) {
    int sourceId = m_breakpoints[i].sourceId;
    int lineno = m_breakpoints[i].lineno;
    if (m_sourceFragments.contains(sourceId) &&
        m_sourceFragments[sourceId]->sourceFile == sourceFile) {
      int absLineno = m_sourceFragments[sourceId]->baseLine+lineno-1;
      if (absLineno >= line0 && absLineno <= line1)
        return true;
    }
  }

  return false;
}

#include "kjs_debugwin.moc"

#endif // KJS_DEBUGGER
