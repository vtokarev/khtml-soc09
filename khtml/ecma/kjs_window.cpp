// -*- c-basic-offset: 2 -*-
/*
 *  This file is part of the KDE libraries
 *  Copyright (C) 2000-2003 Harri Porten (porten@kde.org)
 *  Copyright (C) 2001-2003 David Faure (faure@kde.org)
 *  Copyright (C) 2003 Apple Computer, Inc.
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

#include "kjs_window.h"

#include <khtmlview.h>
#include <khtml_part.h>
#include <khtmlpart_p.h>
#include <khtml_settings.h>
#include <xml/dom2_eventsimpl.h>
#include <xml/dom_docimpl.h>
#include <dom/html_document.h>
#include <misc/htmltags.h>
#include <html/html_documentimpl.h>
#include <rendering/render_frames.h>

#include <config.h>

#include <QtCore/QTimer>
#include <QtGui/QApplication>
#include <kdebug.h>
#include <kmessagebox.h>
#include <kinputdialog.h>
#include <klocale.h>
#include <kcodecs.h>
#include <kparts/browserinterface.h>
#include <kwindowsystem.h>

#ifndef KONQ_EMBEDDED
#include <kbookmarkmanager.h>
#include <kbookmarkdialog.h>
#endif
#include <kglobalsettings.h>
#include <assert.h>
#include <QtGui/QStyle>
#include <QtCore/QObject>
#include <QtGui/QTextDocument>
#include <kstringhandler.h>

#include "kjs_proxy.h"
#include "kjs_navigator.h"
#include "kjs_mozilla.h"
#include "kjs_html.h"
#include "kjs_range.h"
#include "kjs_traversal.h"
#include "kjs_css.h"
#include "kjs_events.h"
#include "kjs_views.h"
#include "kjs_audio.h"
#include "kjs_context2d.h"
#include "xmlhttprequest.h"
#include "xmlserializer.h"
#include "domparser.h"

#include <rendering/render_replaced.h>


using namespace KJS;

namespace KJS {

  class History : public JSObject {
    friend class HistoryFunc;
  public:
    History(ExecState *exec, KHTMLPart *p)
      : JSObject(exec->lexicalInterpreter()->builtinObjectPrototype()), part(p) { }
    virtual bool getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot);
    JSValue *getValueProperty(ExecState *exec, int token) const;
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;
    enum { Back, Forward, Go, Length };
  private:
    QPointer<KHTMLPart> part;
  };

  class External : public JSObject {
    friend class ExternalFunc;
  public:
    External(ExecState *exec, KHTMLPart *p)
      : JSObject(exec->lexicalInterpreter()->builtinObjectPrototype()), part(p) { }
    virtual bool getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot);
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;
    enum { AddFavorite };
  private:
    QPointer<KHTMLPart> part;
  };


  class FrameArray : public JSObject {
  public:
    FrameArray(ExecState *exec, KHTMLPart *p)
      : JSObject(exec->lexicalInterpreter()->builtinObjectPrototype()), part(p) { }
    virtual bool getOwnPropertySlot(ExecState *, const Identifier&, PropertySlot&);
    JSValue *getValueProperty(ExecState *exec, int token);
    virtual UString toString(ExecState *exec) const;
    enum { Length, Location };
    JSValue *indexGetter(ExecState *, unsigned index);
    virtual JSValue *callAsFunction(ExecState *exec, JSObject *thisObj, const List &args);
    virtual bool implementsCall() const { return true; }
  private:
    static JSValue *nameGetter(ExecState *, JSObject*, const Identifier&, const PropertySlot&);
    static JSValue *nameFallBackGetter(ExecState *, JSObject*, const Identifier&, const PropertySlot&);
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;

    QPointer<KHTMLPart> part;
  };

} //namespace KJS

#include "kjs_window.lut.h"

namespace KJS {

////////////////////// Screen Object ////////////////////////

// table for screen object
/*
@begin ScreenTable 7
  height        Screen::Height		DontEnum|ReadOnly
  width         Screen::Width		DontEnum|ReadOnly
  colorDepth    Screen::ColorDepth	DontEnum|ReadOnly
  pixelDepth    Screen::PixelDepth	DontEnum|ReadOnly
  availLeft     Screen::AvailLeft	DontEnum|ReadOnly
  availTop      Screen::AvailTop	DontEnum|ReadOnly
  availHeight   Screen::AvailHeight	DontEnum|ReadOnly
  availWidth    Screen::AvailWidth	DontEnum|ReadOnly
@end
*/

const ClassInfo Screen::info = { "Screen", 0, &ScreenTable, 0 };

// We set the object prototype so that toString is implemented
Screen::Screen(ExecState *exec)
  : JSObject(exec->lexicalInterpreter()->builtinObjectPrototype()) {}

bool Screen::getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot)
{
#ifdef KJS_VERBOSE
  kDebug(6070) << "Screen::getPropertyName " << propertyName.qstring();
#endif
  return getStaticValueSlot<Screen, JSObject>(exec, &ScreenTable, this, propertyName, slot);
}

JSValue *Screen::getValueProperty(ExecState *exec, int token) const
{
  QWidget *thisWidget = Window::retrieveActive(exec)->part()->widget();
  QRect sg = KGlobalSettings::desktopGeometry(thisWidget);

  switch( token ) {
  case Height:
    return jsNumber(sg.height());
  case Width:
    return jsNumber(sg.width());
  case ColorDepth:
  case PixelDepth:
    return jsNumber(thisWidget->depth());
  case AvailLeft: {
#if defined Q_WS_X11 && ! defined K_WS_QTONLY
    QRect clipped = KWindowSystem::workArea().intersect(sg);
    return jsNumber(clipped.x()-sg.x());
#else
    return jsNumber(10);
#endif
  }
  case AvailTop: {
#if defined Q_WS_X11 && ! defined K_WS_QTONLY
    QRect clipped = KWindowSystem::workArea().intersect(sg);
    return jsNumber(clipped.y()-sg.y());
#else
    return jsNumber(10);
#endif
  }
  case AvailHeight: {
#if defined Q_WS_X11 && ! defined K_WS_QTONLY
    QRect clipped = KWindowSystem::workArea().intersect(sg);
    return jsNumber(clipped.height());
#else
    return jsNumber(100);
#endif
  }
  case AvailWidth: {
#if defined Q_WS_X11 && ! defined K_WS_QTONLY
    QRect clipped = KWindowSystem::workArea().intersect(sg);
    return jsNumber(clipped.width());
#else
    return jsNumber(100);
#endif
  }
  default:
    kDebug(6070) << "WARNING: Screen::getValueProperty unhandled token " << token;
    return jsUndefined();
  }
}

////////////////////// Window Object ////////////////////////

const ClassInfo Window::info = { "Window", &DOMAbstractView::info, &WindowTable, 0 };

/*
@begin WindowTable 233
  atob		Window::AToB		DontDelete|Function 1
  btoa		Window::BToA		DontDelete|Function 1
  closed	Window::Closed		DontDelete|ReadOnly
  crypto	Window::Crypto		DontDelete|ReadOnly
  defaultStatus	Window::DefaultStatus	DontDelete
  defaultstatus	Window::DefaultStatus	DontDelete
  status	Window::Status		DontDelete
  document	Window::Document	DontDelete|ReadOnly
  frameElement		Window::FrameElement		DontDelete|ReadOnly
  frames	Window::Frames		DontDelete|ReadOnly
  history	Window::_History	DontDelete|ReadOnly
  external	Window::_External	DontDelete|ReadOnly
  event		Window::Event		DontDelete|ReadOnly
  innerHeight	Window::InnerHeight	DontDelete|ReadOnly
  innerWidth	Window::InnerWidth	DontDelete|ReadOnly
  length	Window::Length		DontDelete|ReadOnly
  location	Window::_Location	DontDelete
  name		Window::Name		DontDelete
  navigator	Window::_Navigator	DontDelete|ReadOnly
  clientInformation	Window::ClientInformation	DontDelete|ReadOnly
  konqueror	Window::_Konqueror	DontDelete|ReadOnly
  offscreenBuffering	Window::OffscreenBuffering	DontDelete|ReadOnly
  opener	Window::Opener		DontDelete|ReadOnly
  outerHeight	Window::OuterHeight	DontDelete|ReadOnly
  outerWidth	Window::OuterWidth	DontDelete|ReadOnly
  pageXOffset	Window::PageXOffset	DontDelete|ReadOnly
  pageYOffset	Window::PageYOffset	DontDelete|ReadOnly
  parent	Window::Parent		DontDelete|ReadOnly
  personalbar	Window::Personalbar	DontDelete|ReadOnly
  screenX	Window::ScreenX		DontDelete|ReadOnly
  screenY	Window::ScreenY		DontDelete|ReadOnly
  scrollbars	Window::Scrollbars	DontDelete|ReadOnly
  scroll	Window::Scroll		DontDelete|Function 2
  scrollBy	Window::ScrollBy	DontDelete|Function 2
  scrollTo	Window::ScrollTo	DontDelete|Function 2
  scrollX       Window::ScrollX         DontDelete|ReadOnly
  scrollY       Window::ScrollY         DontDelete|ReadOnly
  moveBy	Window::MoveBy		DontDelete|Function 2
  moveTo	Window::MoveTo		DontDelete|Function 2
  resizeBy	Window::ResizeBy	DontDelete|Function 2
  resizeTo	Window::ResizeTo	DontDelete|Function 2
  self		Window::Self		DontDelete|ReadOnly
  window	Window::_Window		DontDelete|ReadOnly
  top		Window::Top		DontDelete|ReadOnly
  screen	Window::_Screen		DontDelete|ReadOnly
  alert		Window::Alert		DontDelete|Function 1
  confirm	Window::Confirm		DontDelete|Function 1
  prompt	Window::Prompt		DontDelete|Function 2
  open		Window::Open		DontDelete|Function 3
  setTimeout	Window::SetTimeout	DontDelete|Function 2
  clearTimeout	Window::ClearTimeout	DontDelete|Function 1
  focus		Window::Focus		DontDelete|Function 0
  blur		Window::Blur		DontDelete|Function 0
  close		Window::Close		DontDelete|Function 0
  setInterval	Window::SetInterval	DontDelete|Function 2
  clearInterval	Window::ClearInterval	DontDelete|Function 1
  captureEvents	Window::CaptureEvents	DontDelete|Function 0
  releaseEvents	Window::ReleaseEvents	DontDelete|Function 0
  print		Window::Print		DontDelete|Function 0
  addEventListener	Window::AddEventListener	DontDelete|Function 3
  removeEventListener	Window::RemoveEventListener	DontDelete|Function 3
# Normally found in prototype. Add to window object itself to make them
# accessible in closed and cross-site windows
  valueOf       Window::ValueOf		DontDelete|Function 0
  toString      Window::ToString	DontDelete|Function 0
# IE extension
  navigate	Window::Navigate	DontDelete|Function 1
# Mozilla extension
  sidebar	Window::SideBar		DontDelete|ReadOnly
  getComputedStyle	Window::GetComputedStyle	DontDelete|Function 2

# Warning, when adding a function to this object you need to add a case in Window::get

# Event handlers
# IE also has: onactivate, onbefore/afterprint, onbeforedeactivate/unload, oncontrolselect,
# ondeactivate, onhelp, onmovestart/end, onresizestart/end, onscroll.
# It doesn't have onabort, onchange, ondragdrop (but NS has that last one).
  onabort	Window::Onabort		DontDelete
  onblur	Window::Onblur		DontDelete
  onchange	Window::Onchange	DontDelete
  onclick	Window::Onclick		DontDelete
  ondblclick	Window::Ondblclick	DontDelete
  ondragdrop	Window::Ondragdrop	DontDelete
  onerror	Window::Onerror		DontDelete
  onfocus	Window::Onfocus		DontDelete
  onkeydown	Window::Onkeydown	DontDelete
  onkeypress	Window::Onkeypress	DontDelete
  onkeyup	Window::Onkeyup		DontDelete
  onload	Window::Onload		DontDelete
  onmousedown	Window::Onmousedown	DontDelete
  onmousemove	Window::Onmousemove	DontDelete
  onmouseout	Window::Onmouseout	DontDelete
  onmouseover	Window::Onmouseover	DontDelete
  onmouseup	Window::Onmouseup	DontDelete
  onmove	Window::Onmove		DontDelete
  onreset	Window::Onreset		DontDelete
  onresize	Window::Onresize	DontDelete
  onselect	Window::Onselect	DontDelete
  onsubmit	Window::Onsubmit	DontDelete
  onunload	Window::Onunload	DontDelete

# Constructors/constant tables
  Node		Window::Node		DontEnum|DontDelete
  Event		Window::EventCtor	DontEnum|DontDelete
  Range		Window::Range		DontEnum|DontDelete
  NodeFilter	Window::NodeFilter	DontEnum|DontDelete
  NodeList	Window::NodeList	DontEnum|DontDelete
  DOMException	Window::DOMException	DontEnum|DontDelete
  RangeException Window::RangeException	DontEnum|DontDelete
  CSSRule	Window::CSSRule		DontEnum|DontDelete
  MutationEvent Window::MutationEventCtor   DontEnum|DontDelete
  KeyboardEvent Window::KeyboardEventCtor   DontEnum|DontDelete
  EventException Window::EventExceptionCtor DontEnum|DontDelete
  Audio		Window::Audio		DontEnum|DontDelete|ReadOnly
  Image		Window::Image		DontEnum|DontDelete|ReadOnly
  Option	Window::Option		DontEnum|DontDelete|ReadOnly
  XMLHttpRequest Window::XMLHttpRequest DontEnum|DontDelete|ReadOnly
  XMLSerializer	Window::XMLSerializer	DontEnum|DontDelete|ReadOnly
  DOMParser	Window::DOMParser	DontEnum|DontDelete|ReadOnly

# Mozilla dom emulation ones.
  Element   Window::ElementCtor DontEnum|DontDelete
  Document  Window::DocumentCtor DontEnum|DontDelete
  #this one is an alias since we don't have a separate XMLDocument
  XMLDocument Window::DocumentCtor DontEnum|DontDelete
  HTMLElement  Window::HTMLElementCtor DontEnum|DontDelete
  HTMLDocument  Window::HTMLDocumentCtor DontEnum|DontDelete
  HTMLHtmlElement Window::HTMLHtmlElementCtor DontEnum|DontDelete
  HTMLHeadElement Window::HTMLHeadElementCtor DontEnum|DontDelete
  HTMLLinkElement Window::HTMLLinkElementCtor DontEnum|DontDelete
  HTMLTitleElement Window::HTMLTitleElementCtor DontEnum|DontDelete
  HTMLMetaElement Window::HTMLMetaElementCtor DontEnum|DontDelete
  HTMLBaseElement Window::HTMLBaseElementCtor DontEnum|DontDelete
  HTMLIsIndexElement Window::HTMLIsIndexElementCtor DontEnum|DontDelete
  HTMLStyleElement Window::HTMLStyleElementCtor DontEnum|DontDelete
  HTMLBodyElement Window::HTMLBodyElementCtor DontEnum|DontDelete
  HTMLFormElement Window::HTMLFormElementCtor DontEnum|DontDelete
  HTMLSelectElement Window::HTMLSelectElementCtor DontEnum|DontDelete
  HTMLOptGroupElement Window::HTMLOptGroupElementCtor DontEnum|DontDelete
  HTMLOptionElement Window::HTMLOptionElementCtor DontEnum|DontDelete
  HTMLInputElement Window::HTMLInputElementCtor DontEnum|DontDelete
  HTMLTextAreaElement Window::HTMLTextAreaElementCtor DontEnum|DontDelete
  HTMLButtonElement Window::HTMLButtonElementCtor DontEnum|DontDelete
  HTMLLabelElement Window::HTMLLabelElementCtor DontEnum|DontDelete
  HTMLFieldSetElement Window::HTMLFieldSetElementCtor DontEnum|DontDelete
  HTMLLegendElement Window::HTMLLegendElementCtor DontEnum|DontDelete
  HTMLUListElement Window::HTMLUListElementCtor DontEnum|DontDelete
  HTMLOListElement Window::HTMLOListElementCtor DontEnum|DontDelete
  HTMLDListElement Window::HTMLDListElementCtor DontEnum|DontDelete
  HTMLDirectoryElement Window::HTMLDirectoryElementCtor DontEnum|DontDelete
  HTMLMenuElement Window::HTMLMenuElementCtor DontEnum|DontDelete
  HTMLLIElement Window::HTMLLIElementCtor DontEnum|DontDelete
  HTMLDivElement Window::HTMLDivElementCtor DontEnum|DontDelete
  HTMLParagraphElement Window::HTMLParagraphElementCtor DontEnum|DontDelete
  HTMLHeadingElement Window::HTMLHeadingElementCtor DontEnum|DontDelete
  HTMLBlockQuoteElement Window::HTMLBlockQuoteElementCtor DontEnum|DontDelete
  HTMLQuoteElement Window::HTMLQuoteElementCtor DontEnum|DontDelete
  HTMLPreElement Window::HTMLPreElementCtor DontEnum|DontDelete
  HTMLBRElement Window::HTMLBRElementCtor DontEnum|DontDelete
  HTMLBaseFontElement Window::HTMLBaseFontElementCtor DontEnum|DontDelete
  HTMLFontElement Window::HTMLFontElementCtor DontEnum|DontDelete
  HTMLHRElement Window::HTMLHRElementCtor DontEnum|DontDelete
  HTMLModElement Window::HTMLModElementCtor DontEnum|DontDelete
  HTMLAnchorElement Window::HTMLAnchorElementCtor DontEnum|DontDelete
  HTMLImageElement Window::HTMLImageElementCtor DontEnum|DontDelete
  HTMLObjectElement Window::HTMLObjectElementCtor DontEnum|DontDelete
  HTMLParamElement Window::HTMLParamElementCtor DontEnum|DontDelete
  HTMLAppletElement Window::HTMLAppletElementCtor DontEnum|DontDelete
  HTMLMapElement Window::HTMLMapElementCtor DontEnum|DontDelete
  HTMLAreaElement Window::HTMLAreaElementCtor DontEnum|DontDelete
  HTMLScriptElement Window::HTMLScriptElementCtor DontEnum|DontDelete
  HTMLTableElement Window::HTMLTableElementCtor DontEnum|DontDelete
  HTMLTableCaptionElement Window::HTMLTableCaptionElementCtor DontEnum|DontDelete
  HTMLTableColElement Window::HTMLTableColElementCtor DontEnum|DontDelete
  HTMLTableSectionElement Window::HTMLTableSectionElementCtor DontEnum|DontDelete
  HTMLTableRowElement Window::HTMLTableRowElementCtor DontEnum|DontDelete
  HTMLTableCellElement Window::HTMLTableCellElementCtor DontEnum|DontDelete
  HTMLFrameSetElement Window::HTMLFrameSetElementCtor DontEnum|DontDelete
  HTMLLayerElement Window::HTMLLayerElementCtor DontEnum|DontDelete
  HTMLFrameElement Window::HTMLFrameElementCtor DontEnum|DontDelete
  HTMLIFrameElement Window::HTMLIFrameElementCtor DontEnum|DontDelete
  HTMLCollection Window::HTMLCollectionCtor DontEnum|DontDelete
  HTMLCanvasElement Window::HTMLCanvasElementCtor DontEnum|DontDelete
  CSSStyleDeclaration Window::CSSStyleDeclarationCtor DontEnum|DontDelete
  CanvasRenderingContext2D Window::Context2DCtor DontEnum|DontDelete
@end
*/
KJS_IMPLEMENT_PROTOFUNC(WindowFunc)

Window::Window(khtml::ChildFrame *p)
  : JSObject(/*no proto*/), m_frame(p), screen(0), history(0), external(0), m_frames(0), loc(0), m_evt(0)
{
  winq = new WindowQObject(this);
  //kDebug(6070) << "Window::Window this=" << this << " part=" << m_part << " " << m_part->name();
}

Window::~Window()
{
  delete winq;
}

Window *Window::retrieveWindow(KParts::ReadOnlyPart *p)
{
  JSObject *obj = retrieve( p )->getObject();
#ifndef NDEBUG
  // obj should never be null, except when javascript has been disabled in that part.
  KHTMLPart *part = qobject_cast<KHTMLPart*>(p);
  if ( part && part->jScriptEnabled() )
  {
    assert( obj );
#ifndef QWS
    assert( dynamic_cast<KJS::Window*>(obj) ); // type checking
#endif
  }
#endif
  if ( !obj ) // JS disabled
    return 0;
  return static_cast<KJS::Window*>(obj);
}

Window *Window::retrieveActive(ExecState *exec)
{
  JSValue *imp = exec->dynamicInterpreter()->globalObject();
  assert( imp );
#ifndef QWS
  assert( dynamic_cast<KJS::Window*>(imp) );
#endif
  return static_cast<KJS::Window*>(imp);
}

JSValue *Window::retrieve(KParts::ReadOnlyPart *p)
{
  assert(p);
  KHTMLPart * part = qobject_cast<KHTMLPart*>(p);
  KJSProxy *proxy = 0L;
  if (!part) {
    part = qobject_cast<KHTMLPart*>(p->parent());
    if (part)
      proxy = part->framejScript(p);
  } else
    proxy = part->jScript();
  if (proxy) {
#ifdef KJS_VERBOSE
    kDebug(6070) << "Window::retrieve part=" << part << " '" << part->objectName() << "' interpreter=" << proxy->interpreter() << " window=" << proxy->interpreter()->globalObject();
#endif
    return proxy->interpreter()->globalObject(); // the Global object is the "window"
  } else {
#ifdef KJS_VERBOSE
    kDebug(6070) << "Window::retrieve part=" << p << " '" << p->objectName() << "' no jsproxy.";
#endif
    return jsUndefined(); // This can happen with JS disabled on the domain of that window
  }
}

Location *Window::location() const
{
  if (!loc)
    const_cast<Window*>(this)->loc = new Location(m_frame);
  return loc;
}

JSObject* Window::frames( ExecState* exec ) const
{
  KHTMLPart *part = qobject_cast<KHTMLPart*>(m_frame->m_part);
  if (part)
    return m_frames ? m_frames :
      (const_cast<Window*>(this)->m_frames = new FrameArray(exec, part));
  return 0L;
}

// reference our special objects during garbage collection
void Window::mark()
{
  JSObject::mark();
  if (screen && !screen->marked())
    screen->mark();
  if (history && !history->marked())
    history->mark();
  if (external && !external->marked())
    external->mark();
  if (m_frames && !m_frames->marked())
    m_frames->mark();
  //kDebug(6070) << "Window::mark " << this << " marking loc=" << loc;
  if (loc && !loc->marked())
    loc->mark();
  if (winq)
    winq->mark();
}

UString Window::toString(ExecState *) const
{
  return "[object Window]";
}

bool Window::getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot)
{
#ifdef KJS_VERBOSE
  kDebug(6070) << "Window("<<this<<")::getOwnPropertySlot " << propertyName.qstring();
#endif

  // we want only limited operations on a closed window
  if (m_frame.isNull() || m_frame->m_part.isNull()) {
    const HashEntry* entry = Lookup::findEntry(&WindowTable, propertyName);
    if (entry) {
      switch (entry->value) {
      case Closed:
      case _Location:
      case ValueOf:
      case ToString:
	getSlotFromEntry<WindowFunc, Window>(entry, this, slot);
	return true;
      default:
	break;
      }
    }
    slot.setUndefined(this);
    return true;
  }

  // Look for overrides first
  JSValue **val = getDirectLocation(propertyName);
  if (val) {
    if (isSafeScript(exec))
      fillDirectLocationSlot(slot, val);
    else
      slot.setUndefined(this);
    return true;
  }

  const HashEntry* entry = Lookup::findEntry(&WindowTable, propertyName);
  KHTMLPart *part = qobject_cast<KHTMLPart*>(m_frame->m_part);

  // properties that work on all windows
  if (entry) {
    // ReadOnlyPart first

    switch(entry->value) {
    case Closed:
    case _Location: // No isSafeScript test here, we must be able to _set_ location.href (#49819)
    case _Window:
    case Self:
      getSlotFromEntry<WindowFunc, Window>(entry, this, slot);
      return true;
    default:
        break;
    }
    if (!part) {
        slot.setUndefined(this);
        return true;
    }

    // KHTMLPart next
    switch(entry->value) {
    case Frames:
    case Opener:
    case Parent:
    case Top:
    case Alert:
    case Confirm:
    case Prompt:
    case Open:
    case Close:
    case Focus:
    case Blur:
    case AToB:
    case BToA:
    case ValueOf:
    case ToString:
      getSlotFromEntry<WindowFunc, Window>(entry, this, slot);
      return true;
    default:
      break;
    }
  } else if (!part) {
    // not a  KHTMLPart
    QString rvalue;
    KParts::LiveConnectExtension::Type rtype;
    unsigned long robjid;
    if (m_frame->m_liveconnect &&
        isSafeScript(exec) &&
        m_frame->m_liveconnect->get(0, propertyName.qstring(), rtype, robjid, rvalue))
      return getImmediateValueSlot(this,
                       getLiveConnectValue(m_frame->m_liveconnect, propertyName.qstring(), rtype, rvalue, robjid), slot);

    slot.setUndefined(this);
    return true;
  }

  // properties that only work on safe windows - we can handle them now..
  if (isSafeScript(exec) &&  entry)
  {
    // Disabled in NS-compat mode. Supported by default - can't hurt, unless someone uses
    // if (navigate) to test for IE (unlikely).
    if (entry->value == Navigate && exec->dynamicInterpreter()->compatMode() == Interpreter::NetscapeCompat ) {
      slot.setUndefined(this);
      return true;
    }

    getSlotFromEntry<WindowFunc, Window>(entry, this, slot);
    return true;
  }

  KParts::ReadOnlyPart *rop = part->findFramePart( propertyName.qstring() );

  if (rop) {
    slot.setCustom(this, framePartGetter);
    return true;
  }

  // allow window[1] or parent[1] etc. (#56983)
  bool ok;
  unsigned int i = propertyName.toArrayIndex(&ok);
  if (ok && frameByIndex(i)) {
    slot.setCustomIndex(this, i, indexGetterAdapter<Window>);
    return true;
  }

  // allow shortcuts like 'Image1' instead of document.images.Image1
  DOM::DocumentImpl *doc = part->xmlDocImpl();
  if (isSafeScript(exec) && doc && doc->isHTMLDocument()) {
    DOM::ElementMappingCache::ItemInfo* info = doc ->underDocNamedCache().get(propertyName.qstring());
    if (info || doc->getElementById(propertyName.domString())) {
      slot.setCustom(this, namedItemGetter);
      return true;
    }
  }

  // This isn't necessarily a bug. Some code uses if(!window.blah) window.blah=1
  // But it can also mean something isn't loaded or implemented, hence the WARNING to help grepping.
#ifdef KJS_VERBOSE
  kDebug(6070) << "WARNING: Window::get property not found: " << propertyName.qstring();
#endif

  return JSObject::getOwnPropertySlot(exec, propertyName, slot);
}

KParts::ReadOnlyPart* Window::frameByIndex(unsigned i)
{
  KHTMLPart *part = qobject_cast<KHTMLPart*>(m_frame->m_part);
  QList<KParts::ReadOnlyPart*> frames = part->frames();
  unsigned int len = frames.count();
  if (i < len) {
    KParts::ReadOnlyPart* frame = frames.at(i);
    return frame;
  }
  return 0;
}

JSValue* Window::indexGetter(ExecState *exec, unsigned index)
{
  KParts::ReadOnlyPart* frame = frameByIndex(index);
  if (frame)
    return Window::retrieve(frame);
  return jsUndefined(); //### ?
}

JSValue *Window::framePartGetter(ExecState *exec, JSObject*, const Identifier& propertyName, const PropertySlot& slot)
{
  Window* thisObj = static_cast<Window*>(slot.slotBase());
  KHTMLPart *part = qobject_cast<KHTMLPart*>(thisObj->m_frame->m_part);
  KParts::ReadOnlyPart *rop = part->findFramePart( propertyName.qstring() );
  return thisObj->retrieve(rop);
}

JSValue *Window::namedItemGetter(ExecState *exec, JSObject*, const Identifier& p, const PropertySlot& slot)
{
  Window* thisObj = static_cast<Window*>(slot.slotBase());
  KHTMLPart *part = qobject_cast<KHTMLPart*>(thisObj->m_frame->m_part);
  DOM::DocumentImpl* doc = part->xmlDocImpl();

  DOM::ElementMappingCache::ItemInfo* info = doc->underDocNamedCache().get(p.qstring());
  if (info) {
    if (info->nd)
      return getDOMNode(exec, info->nd);
    else {
      //No cached mapping, do it by hand...
      DOM::HTMLMappedNameCollectionImpl* coll = new DOM::HTMLMappedNameCollectionImpl(doc,
                              DOM::HTMLCollectionImpl::DOCUMENT_NAMED_ITEMS, p.domString());

      if (coll->length() == 1) {
          info->nd = static_cast<DOM::ElementImpl*>(coll->firstItem());
          delete coll;
          return getDOMNode(exec, info->nd);
      }
      return getHTMLCollection(exec, coll);
    }
  }

  DOM::ElementImpl* element = doc->getElementById(p.domString());
  return getDOMNode(exec, element);
}

JSValue* Window::getValueProperty(ExecState *exec, int token) const
{
  KHTMLPart *part = m_frame.isNull() ? 0 : qobject_cast<KHTMLPart*>(m_frame->m_part);
  if (!part) {
    switch (token) {
    case Closed:
      return jsBoolean(true);
    case _Location:
      return jsNull();
    default:
      return jsUndefined();
    }
  }

  switch(token) {
    case Closed:
      return jsBoolean(!part);
    case _Location:
        // No isSafeScript test here, we must be able to _set_ location.href (#49819)
      return location();
    case _Window:
    case Self:
      return retrieve(part);
    case Frames:
      return frames(exec);
    case Opener:
      if (!part->opener())
        return jsNull();    // ### a null Window might be better, but == null
      else                // doesn't work yet
        return retrieve(part->opener());
    case Parent:
      return retrieve(part && part->parentPart() ? part->parentPart() : (KHTMLPart*)part);
    case Top: {
      KHTMLPart *p = part;
      while (p->parentPart())
        p = p->parentPart();
      return retrieve(p);
    }
    case Crypto:
      return jsUndefined(); // ###
    case DefaultStatus:
      return jsString(UString(part->jsDefaultStatusBarText()));
    case Status:
      return jsString(UString(part->jsStatusBarText()));
    case Document:
      if (!part->xmlDocImpl()) {
        kDebug(6070) << "Document.write: adding <HTML><BODY> to create document";
        part->begin();
        part->write("<HTML><BODY>");
        part->end();
      }
      return getDOMNode(exec, part->xmlDocImpl());
    case FrameElement:
      if (m_frame->m_partContainerElement)
        return getDOMNode(exec,m_frame->m_partContainerElement);
      else
        return jsUndefined();
    case Node:
      return NodeConstructor::self(exec);
    case Range:
      return getRangeConstructor(exec);
    case NodeFilter:
      return getNodeFilterConstructor(exec);
    case NodeList:
      return NodeListPseudoCtor::self(exec);
    case DOMException:
      return getDOMExceptionConstructor(exec);
    case RangeException:
      return RangeExceptionPseudoCtor::self(exec);
    case CSSRule:
      return getCSSRuleConstructor(exec);
    case ElementCtor:
      return ElementPseudoCtor::self(exec);
    case HTMLElementCtor:
      return HTMLElementPseudoCtor::self(exec);
    case HTMLHtmlElementCtor:
      return HTMLHtmlElementPseudoCtor::self(exec);
    case HTMLHeadElementCtor:
      return HTMLHeadElementPseudoCtor::self(exec);
    case HTMLLinkElementCtor:
      return HTMLLinkElementPseudoCtor::self(exec);
    case HTMLTitleElementCtor:
      return HTMLTitleElementPseudoCtor::self(exec);
    case HTMLMetaElementCtor:
      return HTMLMetaElementPseudoCtor::self(exec);
    case HTMLBaseElementCtor:
      return HTMLBaseElementPseudoCtor::self(exec);
    case HTMLIsIndexElementCtor:
      return HTMLIsIndexElementPseudoCtor::self(exec);
    case HTMLStyleElementCtor:
      return HTMLStyleElementPseudoCtor::self(exec);
    case HTMLBodyElementCtor:
      return HTMLBodyElementPseudoCtor::self(exec);
    case HTMLFormElementCtor:
      return HTMLFormElementPseudoCtor::self(exec);
    case HTMLSelectElementCtor:
      return HTMLSelectElementPseudoCtor::self(exec);
    case HTMLOptGroupElementCtor:
      return HTMLOptGroupElementPseudoCtor::self(exec);
    case HTMLOptionElementCtor:
      return HTMLOptionElementPseudoCtor::self(exec);
    case HTMLInputElementCtor:
      return HTMLInputElementPseudoCtor::self(exec);
    case HTMLTextAreaElementCtor:
      return HTMLTextAreaElementPseudoCtor::self(exec);
    case HTMLButtonElementCtor:
      return HTMLButtonElementPseudoCtor::self(exec);
    case HTMLLabelElementCtor:
      return HTMLLabelElementPseudoCtor::self(exec);
    case HTMLFieldSetElementCtor:
      return HTMLFieldSetElementPseudoCtor::self(exec);
    case HTMLLegendElementCtor:
      return HTMLLegendElementPseudoCtor::self(exec);
    case HTMLUListElementCtor:
      return HTMLUListElementPseudoCtor::self(exec);
    case HTMLOListElementCtor:
      return HTMLOListElementPseudoCtor::self(exec);
    case HTMLDListElementCtor:
      return HTMLDListElementPseudoCtor::self(exec);
    case HTMLDirectoryElementCtor:
      return HTMLDirectoryElementPseudoCtor::self(exec);
    case HTMLMenuElementCtor:
      return HTMLMenuElementPseudoCtor::self(exec);
    case HTMLLIElementCtor:
      return HTMLLIElementPseudoCtor::self(exec);
    case HTMLDivElementCtor:
      return HTMLDivElementPseudoCtor::self(exec);
    case HTMLParagraphElementCtor:
      return HTMLParagraphElementPseudoCtor::self(exec);
    case HTMLHeadingElementCtor:
      return HTMLHeadingElementPseudoCtor::self(exec);
    case HTMLBlockQuoteElementCtor:
      return HTMLBlockQuoteElementPseudoCtor::self(exec);
    case HTMLQuoteElementCtor:
      return HTMLQuoteElementPseudoCtor::self(exec);
    case HTMLPreElementCtor:
      return HTMLPreElementPseudoCtor::self(exec);
    case HTMLBRElementCtor:
      return HTMLBRElementPseudoCtor::self(exec);
    case HTMLBaseFontElementCtor:
      return HTMLBaseFontElementPseudoCtor::self(exec);
    case HTMLFontElementCtor:
      return HTMLFontElementPseudoCtor::self(exec);
    case HTMLHRElementCtor:
      return HTMLHRElementPseudoCtor::self(exec);
    case HTMLModElementCtor:
      return HTMLModElementPseudoCtor::self(exec);
    case HTMLAnchorElementCtor:
      return HTMLAnchorElementPseudoCtor::self(exec);
    case HTMLImageElementCtor:
      return HTMLImageElementPseudoCtor::self(exec);
    case HTMLObjectElementCtor:
      return HTMLObjectElementPseudoCtor::self(exec);
    case HTMLParamElementCtor:
      return HTMLParamElementPseudoCtor::self(exec);
    case HTMLAppletElementCtor:
      return HTMLAppletElementPseudoCtor::self(exec);
    case HTMLMapElementCtor:
      return HTMLMapElementPseudoCtor::self(exec);
    case HTMLAreaElementCtor:
      return HTMLAreaElementPseudoCtor::self(exec);
    case HTMLScriptElementCtor:
      return HTMLScriptElementPseudoCtor::self(exec);
    case HTMLTableElementCtor:
      return HTMLTableElementPseudoCtor::self(exec);
    case HTMLTableCaptionElementCtor:
      return HTMLTableCaptionElementPseudoCtor::self(exec);
    case HTMLTableColElementCtor:
      return HTMLTableColElementPseudoCtor::self(exec);
    case HTMLTableSectionElementCtor:
      return HTMLTableSectionElementPseudoCtor::self(exec);
    case HTMLTableRowElementCtor:
      return HTMLTableRowElementPseudoCtor::self(exec);
    case HTMLTableCellElementCtor:
      return HTMLTableCellElementPseudoCtor::self(exec);
    case HTMLFrameSetElementCtor:
      return HTMLFrameSetElementPseudoCtor::self(exec);
    case HTMLLayerElementCtor:
      return HTMLLayerElementPseudoCtor::self(exec);
    case HTMLFrameElementCtor:
      return HTMLFrameElementPseudoCtor::self(exec);
    case HTMLIFrameElementCtor:
      return HTMLIFrameElementPseudoCtor::self(exec);
    case HTMLCollectionCtor:
      return HTMLCollectionPseudoCtor::self(exec);
    case HTMLCanvasElementCtor:
      return HTMLCanvasElementPseudoCtor::self(exec);
    case Context2DCtor:
      return Context2DPseudoCtor::self(exec);
    case DocumentCtor:
      return DocumentPseudoCtor::self(exec);
    case HTMLDocumentCtor:
      return HTMLDocumentPseudoCtor::self(exec);
    case CSSStyleDeclarationCtor:
        return CSSStyleDeclarationPseudoCtor::self(exec);
    case EventCtor:
      return EventConstructor::self(exec);
    case MutationEventCtor:
      return getMutationEventConstructor(exec);
    case KeyboardEventCtor:
      return getKeyboardEventConstructor(exec);
    case EventExceptionCtor:
      return getEventExceptionConstructor(exec);
    case _History:
      return history ? history :
                   (const_cast<Window*>(this)->history = new History(exec,part));

    case _External:
      return external ? external :
                   (const_cast<Window*>(this)->external = new External(exec,part));

    case Event:
      if (m_evt)
        return getDOMEvent(exec,m_evt);
      else {
#ifdef KJS_VERBOSE
        kDebug(6070) << "WARNING: window(" << this << "," << part->objectName() << ").event, no event!";
#endif
        return jsUndefined();
      }
    case InnerHeight:
    {
      if (!part->view())
        return jsUndefined();
      khtml::RenderWidget::flushWidgetResizes(); // make sure frames have their final size
      int ret = part->view()->visibleHeight();
      // match Gecko which does not subtract the scrollbars
      if (part->view()->horizontalScrollBar()->isVisible()) {
          ret += part->view()->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
          int lvs = part->view()->style()->pixelMetric(QStyle::PM_LayoutVerticalSpacing);
          if (lvs > 0)
              ret += lvs;
      }
      return jsNumber(ret);
    }
    case InnerWidth:
    {
      if (!part->view())
        return jsUndefined();
      khtml::RenderWidget::flushWidgetResizes(); // make sure frames have their final size
      int ret = part->view()->visibleWidth();
      // match Gecko which does not subtract the scrollbars
      if (part->view()->verticalScrollBar()->isVisible()) {
          ret += part->view()->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
          int lhs = part->view()->style()->pixelMetric(QStyle::PM_LayoutHorizontalSpacing);
          if (lhs > 0)
              ret += lhs;
      }
      return jsNumber(ret);
    }
    case Length:
      return jsNumber(part->frames().count());
    case Name:
      return jsString(part->objectName());
    case SideBar:
      return new MozillaSidebarExtension(exec, part);
    case _Navigator:
    case ClientInformation: {
      // Store the navigator in the object so we get the same one each time.
      JSValue *nav( new Navigator(exec, part) );
      const_cast<Window *>(this)->put(exec, "navigator", nav, DontDelete|ReadOnly|Internal);
      const_cast<Window *>(this)->put(exec, "clientInformation", nav, DontDelete|ReadOnly|Internal);
      return nav;
    }

    case OffscreenBuffering:
      return jsBoolean(true);
    case OuterHeight:
    case OuterWidth:
    {
#if defined Q_WS_X11 && ! defined K_WS_QTONLY
      if (!part->widget())
        return jsNumber(0);
      KWindowInfo inf = KWindowSystem::windowInfo(part->widget()->topLevelWidget()->winId(), NET::WMGeometry);
      return jsNumber(token == OuterHeight ?
                    inf.geometry().height() : inf.geometry().width());
#else
      return jsNumber(token == OuterHeight ?
		    part->view()->height() : part->view()->width());
#endif
    }
    case PageXOffset:
      return jsNumber(part->view()->contentsX());
    case PageYOffset:
      return jsNumber(part->view()->contentsY());
    case Personalbar:
      return jsUndefined(); // ###
    case ScreenLeft:
    case ScreenX: {
      if (!part->view())
        return jsUndefined();
      QRect sg = KGlobalSettings::desktopGeometry(part->view());
      return jsNumber(part->view()->mapToGlobal(QPoint(0,0)).x() + sg.x());
    }
    case ScreenTop:
    case ScreenY: {
      if (!part->view())
        return jsUndefined();
      QRect sg = KGlobalSettings::desktopGeometry(part->view());
      return jsNumber(part->view()->mapToGlobal(QPoint(0,0)).y() + sg.y());
    }
    case ScrollX: {
      if (!part->view())
        return jsUndefined();
      return jsNumber(part->view()->contentsX());
    }
    case ScrollY: {
      if (!part->view())
        return jsUndefined();
      return jsNumber(part->view()->contentsY());
    }
    case Scrollbars:
      return jsUndefined(); // ###
    case _Screen:
      return screen ? screen :
                   (const_cast<Window*>(this)->screen = new Screen(exec));
    case Audio:
      return new AudioConstructorImp(exec, part->xmlDocImpl());
    case Image:
      return new ImageConstructorImp(exec, part->xmlDocImpl());
    case Option:
      return new OptionConstructorImp(exec, part->xmlDocImpl());
    case XMLHttpRequest:
      return new XMLHttpRequestConstructorImp(exec, part->xmlDocImpl());
    case XMLSerializer:
      return new XMLSerializerConstructorImp(exec);
    case DOMParser:
      return new DOMParserConstructorImp(exec, part->xmlDocImpl());
    case Onabort:
      return getListener(exec,DOM::EventImpl::ABORT_EVENT);
    case Onblur:
      return getListener(exec,DOM::EventImpl::BLUR_EVENT);
    case Onchange:
      return getListener(exec,DOM::EventImpl::CHANGE_EVENT);
    case Onclick:
      return getListener(exec,DOM::EventImpl::KHTML_ECMA_CLICK_EVENT);
    case Ondblclick:
      return getListener(exec,DOM::EventImpl::KHTML_ECMA_DBLCLICK_EVENT);
    case Ondragdrop:
      return getListener(exec,DOM::EventImpl::KHTML_DRAGDROP_EVENT);
    case Onerror:
      return getListener(exec,DOM::EventImpl::ERROR_EVENT);
    case Onfocus:
      return getListener(exec,DOM::EventImpl::FOCUS_EVENT);
    case Onkeydown:
      return getListener(exec,DOM::EventImpl::KEYDOWN_EVENT);
    case Onkeypress:
      return getListener(exec,DOM::EventImpl::KEYPRESS_EVENT);
    case Onkeyup:
      return getListener(exec,DOM::EventImpl::KEYUP_EVENT);
    case Onload:
      return getListener(exec,DOM::EventImpl::LOAD_EVENT);
    case Onmousedown:
      return getListener(exec,DOM::EventImpl::MOUSEDOWN_EVENT);
    case Onmousemove:
      return getListener(exec,DOM::EventImpl::MOUSEMOVE_EVENT);
    case Onmouseout:
      return getListener(exec,DOM::EventImpl::MOUSEOUT_EVENT);
    case Onmouseover:
      return getListener(exec,DOM::EventImpl::MOUSEOVER_EVENT);
    case Onmouseup:
      return getListener(exec,DOM::EventImpl::MOUSEUP_EVENT);
    case Onmove:
      return getListener(exec,DOM::EventImpl::KHTML_MOVE_EVENT);
    case Onreset:
      return getListener(exec,DOM::EventImpl::RESET_EVENT);
    case Onresize:
      return getListener(exec,DOM::EventImpl::RESIZE_EVENT);
    case Onselect:
      return getListener(exec,DOM::EventImpl::SELECT_EVENT);
    case Onsubmit:
      return getListener(exec,DOM::EventImpl::SUBMIT_EVENT);
    case Onunload:
      return getListener(exec,DOM::EventImpl::UNLOAD_EVENT);
  }

  return jsUndefined();
}

void Window::put(ExecState* exec, const Identifier &propertyName, JSValue *value, int attr)
{
  // we don't want any operations on a closed window
  if (m_frame.isNull() || m_frame->m_part.isNull()) {
    // ### throw exception? allow setting of some props like location?
    return;
  }

  // Called by an internal KJS call (e.g. InterpreterImp's constructor) ?
  // If yes, save time and jump directly to JSObject. We also have
  // to do this now since calling isSafeScript() may not work yet.
  if (attr != None && attr != DontDelete)
  {
    JSObject::put( exec, propertyName, value, attr );
    return;
  }
  

  // If we already have a variable, that's writeable w/o a getter/setter mess, just write to it.
  bool safe = isSafeScript(exec);
  if (safe) {
    if (JSValue** slot = getDirectWriteLocation(propertyName)) {
      *slot = value;
      return;
    }
  }

  const HashEntry* entry = Lookup::findEntry(&WindowTable, propertyName);
  if (entry && !m_frame.isNull() && !m_frame->m_part.isNull())
  {
#ifdef KJS_VERBOSE
    kDebug(6070) << "Window("<<this<<")::put " << propertyName.qstring();
#endif
    switch( entry->value) {
    case _Location:
      goURL(exec, value->toString(exec).qstring(), false /*don't lock history*/);
      return;
    default:
      break;
    }
    KHTMLPart *part = qobject_cast<KHTMLPart*>(m_frame->m_part);
    if (part) {
    switch( entry->value ) {
    case Status: {
      if  (isSafeScript(exec) && part->settings()->windowStatusPolicy(part->url().host())
		== KHTMLSettings::KJSWindowStatusAllow) {
      UString s = value->toString(exec);
      part->setJSStatusBarText(s.qstring());
      }
      return;
    }
    case DefaultStatus: {
      if (isSafeScript(exec) && part->settings()->windowStatusPolicy(part->url().host())
		== KHTMLSettings::KJSWindowStatusAllow) {
      UString s = value->toString(exec);
      part->setJSDefaultStatusBarText(s.qstring());
      }
      return;
    }
    case Onabort:
      if (isSafeScript(exec))
        setListener(exec, DOM::EventImpl::ABORT_EVENT,value);
      return;
    case Onblur:
      if (isSafeScript(exec))
        setListener(exec, DOM::EventImpl::BLUR_EVENT,value);
      return;
    case Onchange:
      if (isSafeScript(exec))
        setListener(exec, DOM::EventImpl::CHANGE_EVENT,value);
      return;
    case Onclick:
      if (isSafeScript(exec))
        setListener(exec,DOM::EventImpl::KHTML_ECMA_CLICK_EVENT,value);
      return;
    case Ondblclick:
      if (isSafeScript(exec))
        setListener(exec,DOM::EventImpl::KHTML_ECMA_DBLCLICK_EVENT,value);
      return;
    case Ondragdrop:
      if (isSafeScript(exec))
        setListener(exec,DOM::EventImpl::KHTML_DRAGDROP_EVENT,value);
      return;
    case Onerror:
      if (isSafeScript(exec))
        setListener(exec,DOM::EventImpl::ERROR_EVENT,value);
      return;
    case Onfocus:
      if (isSafeScript(exec))
        setListener(exec,DOM::EventImpl::FOCUS_EVENT,value);
      return;
    case Onkeydown:
      if (isSafeScript(exec))
        setListener(exec,DOM::EventImpl::KEYDOWN_EVENT,value);
      return;
    case Onkeypress:
      if (isSafeScript(exec))
        setListener(exec,DOM::EventImpl::KEYPRESS_EVENT,value);
      return;
    case Onkeyup:
      if (isSafeScript(exec))
        setListener(exec,DOM::EventImpl::KEYUP_EVENT,value);
      return;
    case Onload:
      if (isSafeScript(exec))
        setListener(exec,DOM::EventImpl::LOAD_EVENT,value);
      return;
    case Onmousedown:
      if (isSafeScript(exec))
        setListener(exec,DOM::EventImpl::MOUSEDOWN_EVENT,value);
      return;
    case Onmousemove:
      if (isSafeScript(exec))
        setListener(exec,DOM::EventImpl::MOUSEMOVE_EVENT,value);
      return;
    case Onmouseout:
      if (isSafeScript(exec))
        setListener(exec,DOM::EventImpl::MOUSEOUT_EVENT,value);
      return;
    case Onmouseover:
      if (isSafeScript(exec))
        setListener(exec,DOM::EventImpl::MOUSEOVER_EVENT,value);
      return;
    case Onmouseup:
      if (isSafeScript(exec))
        setListener(exec,DOM::EventImpl::MOUSEUP_EVENT,value);
      return;
    case Onmove:
      if (isSafeScript(exec))
        setListener(exec,DOM::EventImpl::KHTML_MOVE_EVENT,value);
      return;
    case Onreset:
      if (isSafeScript(exec))
        setListener(exec,DOM::EventImpl::RESET_EVENT,value);
      return;
    case Onresize:
      if (isSafeScript(exec))
        setListener(exec,DOM::EventImpl::RESIZE_EVENT,value);
      return;
    case Onselect:
      if (isSafeScript(exec))
        setListener(exec,DOM::EventImpl::SELECT_EVENT,value);
      return;
    case Onsubmit:
      if (isSafeScript(exec))
        setListener(exec,DOM::EventImpl::SUBMIT_EVENT,value);
      return;
    case Onunload:
      if (isSafeScript(exec))
        setListener(exec,DOM::EventImpl::UNLOAD_EVENT,value);
      return;
    case Name:
      if (isSafeScript(exec))
        part->setObjectName( value->toString(exec).qstring().toLocal8Bit().data() );
      return;
    default:
      break;
    }
    }
  }
  if (m_frame->m_liveconnect &&
      isSafeScript(exec) &&
      m_frame->m_liveconnect->put(0, propertyName.qstring(), value->toString(exec).qstring()))
    return;
  if (safe) {
    //kDebug(6070) << "Window("<<this<<")::put storing " << propertyName.qstring();
    JSObject::put(exec, propertyName, value, attr);
  }
}

bool Window::toBoolean(ExecState *) const
{
  return !m_frame.isNull() && !m_frame->m_part.isNull();
}

DOM::AbstractViewImpl* Window::toAbstractView() const
{
  KHTMLPart *part = ::qobject_cast<KHTMLPart *>(m_frame->m_part);
  if (!part || !part->xmlDocImpl())
    return 0;
  return part->xmlDocImpl()->defaultView();
}

void Window::scheduleClose()
{
  kDebug(6070) << "Window::scheduleClose window.close() " << m_frame;
  Q_ASSERT(winq);
  QTimer::singleShot( 0, winq, SLOT( timeoutClose() ) );
}

void Window::closeNow()
{
  if (m_frame.isNull() || m_frame->m_part.isNull()) {
    kDebug(6070) << "part is deleted already";
  } else {
    KHTMLPart *part = qobject_cast<KHTMLPart*>(m_frame->m_part);
    if (!part) {
      kDebug(6070) << "closeNow on non KHTML part";
    } else {
      //kDebug(6070) << " -> closing window";
      // We want to make sure that window.open won't find this part by name.
      part->setObjectName( QString() );
      part->deleteLater();
      part = 0;
    }
  }
}

void Window::afterScriptExecution()
{
  DOM::DocumentImpl::updateDocumentsRendering();
  const QList<DelayedAction> delayedActions = m_delayed;
  m_delayed.clear();
  QList<DelayedAction>::ConstIterator it = delayedActions.begin();
  for ( ; it != delayedActions.end() ; ++it )
  {
    switch ((*it).actionId) {
    case DelayedClose:
      scheduleClose();
      return; // stop here, in case of multiple actions
    case DelayedGoHistory:
      goHistory( (*it).param.toInt() );
      break;
    case NullAction:
      // FIXME: anything needs to be done here?  This is warning anyways.
      break;
    };
  }
}

bool Window::checkIsSafeScript(KParts::ReadOnlyPart *activePart) const
{
  if (m_frame.isNull() || m_frame->m_part.isNull()) { // part deleted ? can't grant access
    kDebug(6070) << "Window::isSafeScript: accessing deleted part !";
    return false;
  }
  if (!activePart) {
    kDebug(6070) << "Window::isSafeScript: current interpreter's part is 0L!";
    return false;
  }
   if ( activePart == m_frame->m_part ) // Not calling from another frame, no problem.
     return true;

  KHTMLPart *part = qobject_cast<KHTMLPart*>(m_frame->m_part);
  if (!part)
    return true; // not a KHTMLPart

  if ( !part->xmlDocImpl() )
    return true; // allow to access a window that was just created (e.g. with window.open("about:blank"))

  DOM::HTMLDocumentImpl* thisDocument = part->docImpl();
  if ( !thisDocument ) {
    kDebug(6070) << "Window::isSafeScript: trying to access an XML document !?";
    return false;
  }

  KHTMLPart *activeKHTMLPart = qobject_cast<KHTMLPart*>(activePart);
  if (!activeKHTMLPart)
    return true; // not a KHTMLPart

  DOM::HTMLDocumentImpl* actDocument = activeKHTMLPart->docImpl();
  if ( !actDocument ) {
    kDebug(6070) << "Window::isSafeScript: active part has no document!";
    return false;
  }
  DOM::DOMString actDomain = actDocument->domain();
  DOM::DOMString thisDomain = thisDocument->domain();

  if ( actDomain == thisDomain ) {
#ifdef KJS_VERBOSE
    //kDebug(6070) << "JavaScript: access granted, domain is '" << actDomain.string() << "'";
#endif
    return true;
  }

  kDebug(6070) << "WARNING: JavaScript: access denied for current frame '" << actDomain.string() << "' to frame '" << thisDomain.string() << "'";
  // TODO after 3.1: throw security exception (exec->setException())
  return false;
}

void Window::setListener(ExecState *exec, int eventId, JSValue *func)
{
  KHTMLPart *part = qobject_cast<KHTMLPart*>(m_frame->m_part);
  if (!part || !isSafeScript(exec))
    return;
  DOM::DocumentImpl *doc = static_cast<DOM::DocumentImpl*>(part->htmlDocument().handle());
  if (!doc)
    return;

  doc->setHTMLWindowEventListener(eventId,getJSEventListener(func,true));
}

JSValue *Window::getListener(ExecState *exec, int eventId) const
{
  KHTMLPart *part = qobject_cast<KHTMLPart*>(m_frame->m_part);
  if (!part || !isSafeScript(exec))
    return jsUndefined();
  DOM::DocumentImpl *doc = static_cast<DOM::DocumentImpl*>(part->htmlDocument().handle());
  if (!doc)
    return jsUndefined();

  DOM::EventListener *listener = doc->getHTMLWindowEventListener(eventId);
  if (listener && static_cast<JSEventListener*>(listener)->listenerObj())
    return static_cast<JSEventListener*>(listener)->listenerObj();
  else
    return jsNull();
}


JSEventListener *Window::getJSEventListener(JSValue *val, bool html)
{
  // This function is so hot that it's worth coding it directly with imps.
  KHTMLPart *part = qobject_cast<KHTMLPart*>(m_frame->m_part);
  if (!part || val->type() != ObjectType)
    return 0;

  // It's ObjectType, so it must be valid.
  JSObject *listenerObject = val->getObject();
  JSObject *thisObject = listenerObject;

  // 'listener' is not a simple ecma function. (Always use sanity checks: Better safe than sorry!)
  if (!listenerObject->implementsCall() && part && part->jScript() && part->jScript()->interpreter())
  {
    Interpreter *interpreter = part->jScript()->interpreter();

    // 'listener' probably is an EventListener object containing a 'handleEvent' function.
    JSValue *handleEventValue = listenerObject->get(interpreter->globalExec(), Identifier("handleEvent"));
    JSObject *handleEventObject = handleEventValue->getObject();

    if(handleEventObject && handleEventObject->implementsCall())
    {
      thisObject = listenerObject;
      listenerObject = handleEventObject;
    }
  }

  JSEventListener *existingListener = jsEventListeners[QPair<void*, bool>(thisObject, html)];
  if (existingListener) {
    assert( existingListener->isHTMLEventListener() == html );
    return existingListener;
  }

  // Note that the JSEventListener constructor adds it to our jsEventListeners list
  return new JSEventListener(listenerObject, thisObject, this, html);
}

JSLazyEventListener *Window::getJSLazyEventListener(const QString& code, const QString& srcUrl, int line,
                                                    const QString& name, DOM::NodeImpl *node)
{
  return new JSLazyEventListener(code, srcUrl, line, name, this, node);
}

void Window::clear( ExecState *exec )
{
  delete winq;
  winq = 0L;
  // Get rid of everything, those user vars could hold references to DOM nodes
  clearProperties();

  // Ditto for the special subobjects.
  screen   = 0;
  history  = 0;
  external = 0;
  m_frames = 0;
  loc      = 0;

  // Break the dependency between the listeners and their object
  QHashIterator<const QPair<void*, bool>, JSEventListener*> it(jsEventListeners);
  while ( it.hasNext() ) {
    it.next();
    it.value()->clear();
  }

  // Forget about the listeners (the DOM::NodeImpls will delete them)
  jsEventListeners.clear();

  if (m_frame) {
    KJSProxy* proxy = m_frame->m_jscript;
    if (proxy) // i.e. JS not disabled
    {
      winq = new WindowQObject(this);
      // Now recreate a working global object for the next URL that will use us
      KJS::Interpreter *interpreter = proxy->interpreter();
      interpreter->initGlobalObject();
    }
  }
}

void Window::setCurrentEvent( DOM::EventImpl *evt )
{
  m_evt = evt;
  //kDebug(6070) << "Window " << this << " (part=" << m_part << ")::setCurrentEvent m_evt=" << evt;
}

void Window::goURL(ExecState* exec, const QString& url, bool lockHistory)
{
  Window* active = Window::retrieveActive(exec);
  KHTMLPart *part = qobject_cast<KHTMLPart*>(m_frame->m_part);
  KHTMLPart *active_part = qobject_cast<KHTMLPart*>(active->part());
  // Complete the URL using the "active part" (running interpreter)
  if (active_part && part) {
    QString dstUrl = active_part->htmlDocument().completeURL(url).string();
    kDebug(6070) << "Window::goURL dstUrl=" << dstUrl;

    // check if we're allowed to inject javascript
    if ( isSafeScript(exec) ||
        !KHTMLPartPrivate::isJavaScriptURL(dstUrl) )
    part->scheduleRedirection(-1,
                              dstUrl,
                              lockHistory);
  } else if (!part && m_frame->m_partContainerElement) {
    KParts::BrowserExtension *b = KParts::BrowserExtension::childObject(m_frame->m_part);
    if (b)
      emit b->openUrlRequest(m_frame->m_partContainerElement->document()->completeURL(url));
    kDebug() << "goURL for ROPart";
  }
}

void Window::delayedGoHistory( int steps )
{
    m_delayed.append( DelayedAction( DelayedGoHistory, steps ) );
}

void Window::goHistory( int steps )
{
  KHTMLPart *part = qobject_cast<KHTMLPart*>(m_frame->m_part);
  if(!part)
      // TODO history readonlypart
    return;
  KParts::BrowserExtension *ext = part->browserExtension();
  if(!ext)
    return;
  KParts::BrowserInterface *iface = ext->browserInterface();

  if ( !iface )
    return;

  iface->callMethod( "goHistory(int)", steps );
  //emit ext->goHistory(steps);
}

void KJS::Window::resizeTo(QWidget* tl, int width, int height)
{
  KHTMLPart *part = qobject_cast<KHTMLPart*>(m_frame->m_part);
  if(!part)
      // TODO resizeTo readonlypart
    return;
  KParts::BrowserExtension *ext = part->browserExtension();
  if (!ext) {
    kDebug(6070) << "Window::resizeTo found no browserExtension";
    return;
  }

  // Security check: within desktop limits and bigger than 100x100 (per spec)
  if ( width < 100 || height < 100 ) {
    kDebug(6070) << "Window::resizeTo refused, window would be too small ("<<width<<","<<height<<")";
    return;
  }

  QRect sg = KGlobalSettings::desktopGeometry(tl);

  if ( width > sg.width() || height > sg.height() ) {
    kDebug(6070) << "Window::resizeTo refused, window would be too big ("<<width<<","<<height<<")";
    return;
  }

  kDebug(6070) << "resizing to " << width << "x" << height;

  emit ext->resizeTopLevelWidget( width, height );

  // If the window is out of the desktop, move it up/left
  // (maybe we should use workarea instead of sg, otherwise the window ends up below kicker)
  int right = tl->x() + tl->frameGeometry().width();
  int bottom = tl->y() + tl->frameGeometry().height();
  int moveByX = 0;
  int moveByY = 0;
  if ( right > sg.right() )
    moveByX = - right + sg.right(); // always <0
  if ( bottom > sg.bottom() )
    moveByY = - bottom + sg.bottom(); // always <0
  if ( moveByX || moveByY )
    emit ext->moveTopLevelWidget( tl->x() + moveByX , tl->y() + moveByY );
}

JSValue *Window::openWindow(ExecState *exec, const List& args)
{
  KHTMLPart *part = qobject_cast<KHTMLPart*>(m_frame->m_part);
  if (!part)
    return jsUndefined();
  KHTMLView *widget = part->view();
  JSValue *v = args[0];
  QString str;
  if (!v->isUndefinedOrNull())
    str = v->toString(exec).qstring();

  // prepare arguments
  KUrl url;
  if (!str.isEmpty())
  {
    KHTMLPart* p = qobject_cast<KHTMLPart*>(Window::retrieveActive(exec)->m_frame->m_part);
    if ( p )
      url = p->htmlDocument().completeURL(str).string();
    if ( !p ||
         !static_cast<DOM::DocumentImpl*>(p->htmlDocument().handle())->isURLAllowed(url.url()) )
      return jsUndefined();
  }

  KHTMLSettings::KJSWindowOpenPolicy policy =
		part->settings()->windowOpenPolicy(part->url().host());
  if ( policy == KHTMLSettings::KJSWindowOpenAsk ) {
    emit part->browserExtension()->requestFocus(part);
    QString caption;
    if (!part->url().host().isEmpty())
      caption = part->url().host() + " - ";
    caption += i18n( "Confirmation: JavaScript Popup" );
    if ( KMessageBox::questionYesNo(widget,
                                    str.isEmpty() ?
                                    i18n( "This site is requesting to open up a new browser "
                                          "window via JavaScript.\n"
                                          "Do you want to allow this?" ) :
                                    i18n( "<qt>This site is requesting to open<p>%1</p>in a new browser window via JavaScript.<br />"
                                          "Do you want to allow this?</qt>", KStringHandler::csqueeze(Qt::escape(url.prettyUrl()),  100)),
                                    caption, KGuiItem(i18n("Allow")), KGuiItem(i18n("Do Not Allow")) ) == KMessageBox::Yes )
      policy = KHTMLSettings::KJSWindowOpenAllow;
  } else if ( policy == KHTMLSettings::KJSWindowOpenSmart )
  {
    // window.open disabled unless from a key/mouse event
    if (static_cast<ScriptInterpreter *>(exec->dynamicInterpreter())->isWindowOpenAllowed())
      policy = KHTMLSettings::KJSWindowOpenAllow;
  }

  QString frameName = args.size() > 1 ? args[1]->toString(exec).qstring() : QString("_blank");

  v = args[2];
  QString features;
  if (v && v->type() != UndefinedType && v->toString(exec).size() > 0) {
    features = v->toString(exec).qstring();
    // Buggy scripts have ' at beginning and end, cut those
    if (features.startsWith("\'") && features.endsWith("\'"))
      features = features.mid(1, features.length()-2);
  }

  if ( policy != KHTMLSettings::KJSWindowOpenAllow ) {
    if ( url.isEmpty() )
      part->setSuppressedPopupIndicator(true, 0);
    else {
      part->setSuppressedPopupIndicator(true, part);
      m_suppressedWindowInfo.append( SuppressedWindowInfo( url, frameName, features ) );
    }
    return jsUndefined();
  } else {
    return executeOpenWindow(exec, url, frameName, features);
  }
}

JSValue *Window::executeOpenWindow(ExecState *exec, const KUrl& url, const QString& frameName, const QString& features)
{
    KHTMLPart *p = qobject_cast<KHTMLPart *>(m_frame->m_part);
    KHTMLView *widget = p->view();
    KParts::WindowArgs winargs;

    // scan feature argument
    if (!features.isEmpty()) {
      // specifying window params means false defaults
      winargs.setMenuBarVisible(false);
      winargs.setToolBarsVisible(false);
      winargs.setStatusBarVisible(false);
      winargs.setScrollBarsVisible(false);
      const QStringList flist = features.split(',');
      QStringList::ConstIterator it = flist.begin();
      while (it != flist.end()) {
        QString s = *it++;
        QString key, val;
        int pos = s.indexOf('=');
        if (pos >= 0) {
          key = s.left(pos).trimmed().toLower();
          val = s.mid(pos + 1).trimmed().toLower();
          QRect screen = KGlobalSettings::desktopGeometry(widget->topLevelWidget());

          if (key == "left" || key == "screenx") {
            winargs.setX((int)val.toFloat() + screen.x());
            if (winargs.x() < screen.x() || winargs.x() > screen.right())
              winargs.setX(screen.x()); // only safe choice until size is determined
          } else if (key == "top" || key == "screeny") {
            winargs.setY((int)val.toFloat() + screen.y());
            if (winargs.y() < screen.y() || winargs.y() > screen.bottom())
              winargs.setY(screen.y()); // only safe choice until size is determined
          } else if (key == "height") {
            winargs.setHeight((int)val.toFloat() + 2*qApp->style()->pixelMetric( QStyle::PM_DefaultFrameWidth ) + 2);
            if (winargs.height() > screen.height())  // should actually check workspace
              winargs.setHeight(screen.height());
            if (winargs.height() < 100)
              winargs.setHeight(100);
          } else if (key == "width") {
            winargs.setWidth((int)val.toFloat() + 2*qApp->style()->pixelMetric( QStyle::PM_DefaultFrameWidth ) + 2);
            if (winargs.width() > screen.width())    // should actually check workspace
              winargs.setWidth(screen.width());
            if (winargs.width() < 100)
              winargs.setWidth(100);
          } else {
            goto boolargs;
          }
          continue;
        } else {
          // leaving away the value gives true
          key = s.trimmed().toLower();
          val = "1";
        }
      boolargs:
        if (key == "menubar")
          winargs.setMenuBarVisible(val == "1" || val == "yes");
        else if (key == "toolbar")
          winargs.setToolBarsVisible(val == "1" || val == "yes");
        else if (key == "location")  // ### missing in WindowArgs
          winargs.setToolBarsVisible(val == "1" || val == "yes");
        else if (key == "status" || key == "statusbar")
          winargs.setStatusBarVisible(val == "1" || val == "yes");
        else if (key == "scrollbars")
          winargs.setScrollBarsVisible(val == "1" || val == "yes");
        else if (key == "resizable")
          winargs.setResizable(val == "1" || val == "yes");
        else if (key == "fullscreen")
          winargs.setFullScreen(val == "1" || val == "yes");
      }
    }

    KParts::OpenUrlArguments args;
    KParts::BrowserArguments browserArgs;
    browserArgs.frameName = frameName;

    if ( browserArgs.frameName.toLower() == "_top" )
    {
      while ( p->parentPart() )
        p = p->parentPart();
      Window::retrieveWindow(p)->goURL(exec, url.url(), false /*don't lock history*/);
      return Window::retrieve(p);
    }
    if ( browserArgs.frameName.toLower() == "_parent" )
    {
      if ( p->parentPart() )
        p = p->parentPart();
      Window::retrieveWindow(p)->goURL(exec, url.url(), false /*don't lock history*/);
      return Window::retrieve(p);
    }
    if ( browserArgs.frameName.toLower() == "_self")
    {
      Window::retrieveWindow(p)->goURL(exec, url.url(), false /*don't lock history*/);
      return Window::retrieve(p);
    }
    if ( browserArgs.frameName.toLower() == "replace" )
    {
      Window::retrieveWindow(p)->goURL(exec, url.url(), true /*lock history*/);
      return Window::retrieve(p);
    }
    args.setMimeType("text/html");
    args.setActionRequestedByUser(false);

    // request window (new or existing if framename is set)
    KParts::ReadOnlyPart *newPart = 0;
    emit p->browserExtension()->createNewWindow(KUrl(), args, browserArgs, winargs, &newPart);
    if (newPart && qobject_cast<KHTMLPart*>(newPart)) {
      KHTMLPart *khtmlpart = static_cast<KHTMLPart*>(newPart);
      //qDebug("opener set to %p (this Window's part) in new Window %p  (this Window=%p)",part,win,window);
      khtmlpart->setOpener(p);
      khtmlpart->setOpenedByJS(true);
      if (khtmlpart->document().isNull()) {
        khtmlpart->begin();
        khtmlpart->write("<HTML><BODY>");
        khtmlpart->end();
        if ( p->docImpl() ) {
          //kDebug(6070) << "Setting domain to " << p->docImpl()->domain().string();
          khtmlpart->docImpl()->setDomain( p->docImpl()->domain());
          khtmlpart->docImpl()->setBaseURL( p->docImpl()->baseURL() );
        }
      }
      args.setMimeType(QString());
      if (browserArgs.frameName.toLower() == "_blank")
        browserArgs.frameName.clear();
      if (!url.isEmpty())
        emit khtmlpart->browserExtension()->openUrlRequest(url, args, browserArgs);
      return Window::retrieve(khtmlpart); // global object
    } else
      return jsUndefined();
}

void Window::forgetSuppressedWindows()
{
  m_suppressedWindowInfo.clear();
}

void Window::showSuppressedWindows()
{
  KHTMLPart *part = qobject_cast<KHTMLPart*>(m_frame->m_part);
  KJS::Interpreter *interpreter = part->jScript()->interpreter();
  ExecState *exec = interpreter->globalExec();

  QList<SuppressedWindowInfo> suppressedWindowInfo = m_suppressedWindowInfo;
  m_suppressedWindowInfo.clear();
  foreach ( const SuppressedWindowInfo &info, suppressedWindowInfo ) {
    executeOpenWindow(exec, info.url, info.frameName, info.features);
  }
}

JSValue *WindowFunc::callAsFunction(ExecState *exec, JSObject *thisObj, const List &args)
{
  KJS_CHECK_THIS( Window, thisObj );

  // these should work no matter whether the window is already
  // closed or not
  if (id == Window::ValueOf || id == Window::ToString) {
    return jsString("[object Window]");
  }

  Window *window = static_cast<Window *>(thisObj);
  QString str, str2;

  KHTMLPart *part = qobject_cast<KHTMLPart*>(window->m_frame->m_part);
  if (!part)
    return jsUndefined();

  KHTMLView *widget = part->view();
  JSValue *v = args[0];
  UString s;
  if (!v->isUndefinedOrNull()) {
    s = v->toString(exec);
    str = s.qstring();
  }

  QString caption;
  if (part && !part->url().host().isEmpty())
    caption = part->url().host() + " - ";
  caption += "JavaScript"; // TODO: i18n
  // functions that work everywhere
  switch(id) {
  case Window::Alert: {
    TimerPauser pause(exec);
    if (!widget->dialogsAllowed())
      return jsUndefined();
    if ( part && part->xmlDocImpl() )
      part->xmlDocImpl()->updateRendering();
    if ( part )
      emit part->browserExtension()->requestFocus(part);
    KMessageBox::error(widget, Qt::convertFromPlainText(str, Qt::WhiteSpaceNormal), caption);
    return jsUndefined();
  }
  case Window::Confirm: {
    TimerPauser pause(exec);
    if (!widget->dialogsAllowed())
      return jsUndefined();
    if ( part && part->xmlDocImpl() )
      part->xmlDocImpl()->updateRendering();
    if ( part )
      emit part->browserExtension()->requestFocus(part);
    return jsBoolean((KMessageBox::warningYesNo(widget, Qt::convertFromPlainText(str), caption,
                                                KStandardGuiItem::ok(), KStandardGuiItem::cancel()) == KMessageBox::Yes));
  }
  case Window::Prompt: {
    TimerPauser pause(exec);
#ifndef KONQ_EMBEDDED
    if (!widget->dialogsAllowed())
      return jsUndefined();
    if ( part && part->xmlDocImpl() )
      part->xmlDocImpl()->updateRendering();
    if ( part )
      emit part->browserExtension()->requestFocus(part);
    bool ok;
    if (args.size() >= 2)
      str2 = KInputDialog::getText(caption,
                                   Qt::convertFromPlainText(str),
                                   args[1]->toString(exec).qstring(), &ok, widget);
    else
      str2 = KInputDialog::getText(caption,
                                   Qt::convertFromPlainText(str),
                                   QString(), &ok, widget);
    if ( ok )
        return jsString(UString(str2));
    else
        return jsNull();
#else
    return jsUndefined();
#endif
  }
  case Window::GetComputedStyle:  {
       if ( !part || !part->xmlDocImpl() )
         return jsUndefined();
        DOM::NodeImpl* arg0 = toNode(args[0]);
        if (!arg0 || arg0->nodeType() != DOM::Node::ELEMENT_NODE)
          return jsUndefined(); // throw exception?
        else
          return getDOMCSSStyleDeclaration(exec, part->xmlDocImpl()->defaultView()->getComputedStyle(
              static_cast<DOM::ElementImpl*>(arg0), args[1]->toString(exec).domString().implementation()));
      }
  case Window::Open:
    return window->openWindow(exec, args);
  case Window::Close: {
    /* From http://developer.netscape.com/docs/manuals/js/client/jsref/window.htm :
       The close method closes only windows opened by JavaScript using the open method.
       If you attempt to close any other window, a confirm is generated, which
       lets the user choose whether the window closes.
       This is a security feature to prevent "mail bombs" containing self.close().
       However, if the window has only one document (the current one) in its
       session history, the close is allowed without any confirm. This is a
       special case for one-off windows that need to open other windows and
       then dispose of themselves.
    */
    bool doClose = false;
    if (!part->openedByJS())
    {
      // To conform to the SPEC, we only ask if the window
      // has more than one entry in the history (NS does that too).
      History history(exec,part);

      if ( history.get( exec, "length" )->toInt32(exec) <= 1 )
      {
        doClose = true;
      }
      else
      {
        // Can we get this dialog with tabs??? Does it close the window or the tab in that case?
        emit part->browserExtension()->requestFocus(part);
        if ( KMessageBox::questionYesNo( window->part()->widget(),
                                         i18n("Close window?"), i18n("Confirmation Required"),
                                         KStandardGuiItem::close(), KStandardGuiItem::cancel() )
             == KMessageBox::Yes )
          doClose = true;
      }
    }
    else
      doClose = true;

    if (doClose)
    {
      // If this is the current window (the one the interpreter runs in),
      // then schedule a delayed close (so that the script terminates first).
      // But otherwise, close immediately. This fixes w=window.open("","name");w.close();window.open("name");
      if ( Window::retrieveActive(exec) == window ) {
        if (widget) {
          // quit all dialogs of this view
          // this fixes 'setTimeout('self.close()',1000); alert("Hi");' crash
          widget->closeChildDialogs();
        }
        //kDebug() << "scheduling delayed close";
        // We'll close the window at the end of the script execution
        Window* w = const_cast<Window*>(window);
        w->m_delayed.append( Window::DelayedAction( Window::DelayedClose ) );
      } else {
        //kDebug() << "closing NOW";
        (const_cast<Window*>(window))->closeNow();
      }
    }
    return jsUndefined();
  }
  case Window::Navigate:
    window->goURL(exec, args[0]->toString(exec).qstring(), false /*don't lock history*/);
    return jsUndefined();
  case Window::Focus: {
    KHTMLSettings::KJSWindowFocusPolicy policy =
		part->settings()->windowFocusPolicy(part->url().host());
    if(policy == KHTMLSettings::KJSWindowFocusAllow && widget) {
      widget->topLevelWidget()->raise();
#ifdef Q_WS_X11
      KWindowSystem::unminimizeWindow( widget->topLevelWidget()->winId() );
#else
      //TODO
#endif
      widget->activateWindow();
      emit part->browserExtension()->requestFocus(part);
    }
    return jsUndefined();
  }
  case Window::Blur:
    // TODO
    return jsUndefined();
  case Window::BToA:
  case Window::AToB: {
      if (!s.is8Bit())
          return jsUndefined();
       QByteArray  in, out;
       char *binData = s.ascii();
       in = QByteArray( binData, s.size() );
       if (id == Window::AToB)
           out = QByteArray::fromBase64(in);
       else
           out = in.toBase64();
       UChar *d = new UChar[out.size()];
       for (int i = 0; i < out.size(); i++)
           d[i].uc = (uchar) out[i];
       UString ret(d, out.size(), false /*no copy*/);
       return jsString(ret);
  }

  };


  // now unsafe functions..
  if (!window->isSafeScript(exec))
    return jsUndefined();

  switch (id) {
  case Window::ScrollBy:
    if(args.size() == 2 && widget)
      widget->scrollBy(args[0]->toInt32(exec), args[1]->toInt32(exec));
    return jsUndefined();
  case Window::Scroll:
  case Window::ScrollTo:
    if(args.size() == 2 && widget)
      widget->setContentsPos(args[0]->toInt32(exec), args[1]->toInt32(exec));
    return jsUndefined();
  case Window::MoveBy: {
    KHTMLSettings::KJSWindowMovePolicy policy =
		part->settings()->windowMovePolicy(part->url().host());
    if(policy == KHTMLSettings::KJSWindowMoveAllow && args.size() == 2 && widget)
    {
      KParts::BrowserExtension *ext = part->browserExtension();
      if (ext) {
        QWidget * tl = widget->topLevelWidget();
        QRect sg = KGlobalSettings::desktopGeometry(tl);

        QPoint dest = tl->pos() + QPoint( args[0]->toInt32(exec), args[1]->toInt32(exec) );
        // Security check (the spec talks about UniversalBrowserWrite to disable this check...)
        if ( dest.x() >= sg.x() && dest.y() >= sg.x() &&
             dest.x()+tl->width() <= sg.width()+sg.x() &&
             dest.y()+tl->height() <= sg.height()+sg.y() )
          emit ext->moveTopLevelWidget( dest.x(), dest.y() );
      }
    }
    return jsUndefined();
  }
  case Window::MoveTo: {
    KHTMLSettings::KJSWindowMovePolicy policy =
		part->settings()->windowMovePolicy(part->url().host());
    if(policy == KHTMLSettings::KJSWindowMoveAllow && args.size() == 2 && widget)
    {
      KParts::BrowserExtension *ext = part->browserExtension();
      if (ext) {
        QWidget * tl = widget->topLevelWidget();
        QRect sg = KGlobalSettings::desktopGeometry(tl);

        QPoint dest( args[0]->toInt32(exec)+sg.x(), args[1]->toInt32(exec)+sg.y() );
        // Security check (the spec talks about UniversalBrowserWrite to disable this check...)
        if ( dest.x() >= sg.x() && dest.y() >= sg.y() &&
             dest.x()+tl->width() <= sg.width()+sg.x() &&
             dest.y()+tl->height() <= sg.height()+sg.y() )
		emit ext->moveTopLevelWidget( dest.x(), dest.y() );
      }
    }
    return jsUndefined();
  }
  case Window::ResizeBy: {
    KHTMLSettings::KJSWindowResizePolicy policy =
		part->settings()->windowResizePolicy(part->url().host());
    if(policy == KHTMLSettings::KJSWindowResizeAllow
    		&& args.size() == 2 && widget)
    {
      QWidget * tl = widget->topLevelWidget();
      QRect geom = tl->frameGeometry();
      window->resizeTo( tl,
                        geom.width() + args[0]->toInt32(exec),
                        geom.height() + args[1]->toInt32(exec) );
    }
    return jsUndefined();
  }
  case Window::ResizeTo: {
    KHTMLSettings::KJSWindowResizePolicy policy =
               part->settings()->windowResizePolicy(part->url().host());
    if(policy == KHTMLSettings::KJSWindowResizeAllow
               && args.size() == 2 && widget)
    {
      QWidget * tl = widget->topLevelWidget();
      window->resizeTo( tl, args[0]->toInt32(exec), args[1]->toInt32(exec) );
    }
    return jsUndefined();
  }
  case Window::SetTimeout:
  case Window::SetInterval: {
    bool singleShot;
    int i; // timeout interval
    if (args.size() == 0)
      return jsUndefined();
    if (args.size() > 1) {
      singleShot = (id == Window::SetTimeout);
      i = args[1]->toInt32(exec);
    } else {
      // second parameter is missing. Emulate Mozilla behavior.
      singleShot = true;
      i = 4;
    }
    if (v->type() == StringType) {
      int r = (const_cast<Window*>(window))->winq->installTimeout(Identifier(s), i, singleShot );
      return jsNumber(r);
    }
    else if (v->type() == ObjectType && v->getObject()->implementsCall()) {
      JSObject *func = v->getObject();
      List funcArgs;
      ListIterator it = args.begin();
      int argno = 0;
      while (it != args.end()) {
	JSValue *arg = it++;
	if (argno++ >= 2)
	    funcArgs.append(arg);
      }
      if (args.size() < 2)
	funcArgs.append(jsNumber(i));
      int r = (const_cast<Window*>(window))->winq->installTimeout(func, funcArgs, i, singleShot );
      return jsNumber(r);
    }
    else
      return jsUndefined();
  }
  case Window::ClearTimeout:
  case Window::ClearInterval:
    (const_cast<Window*>(window))->winq->clearTimeout(v->toInt32(exec));
    return jsUndefined();
  case Window::Print:
    if ( widget ) {
      // ### TODO emit onbeforeprint event
      widget->print();
      // ### TODO emit onafterprint event
    }
  case Window::CaptureEvents:
  case Window::ReleaseEvents:
    // Do nothing for now. These are NS-specific legacy calls.
    break;
  case Window::AddEventListener: {
        JSEventListener *listener = Window::retrieveActive(exec)->getJSEventListener(args[1]);
        if (listener) {
	    DOM::DocumentImpl* docimpl = static_cast<DOM::DocumentImpl *>(part->document().handle());
            docimpl->addWindowEventListener(EventName::fromString(args[0]->toString(exec).domString()),listener,args[2]->toBoolean(exec));
        }
        return jsUndefined();
    }
  case Window::RemoveEventListener: {
        JSEventListener *listener = Window::retrieveActive(exec)->getJSEventListener(args[1]);
        if (listener) {
	    DOM::DocumentImpl* docimpl = static_cast<DOM::DocumentImpl *>(part->document().handle());
            docimpl->removeWindowEventListener(EventName::fromString(args[0]->toString(exec).domString()),listener,args[2]->toBoolean(exec));
        }
        return jsUndefined();
    }

  }
  return jsUndefined();
}

////////////////////// ScheduledAction ////////////////////////

// KDE 4: Make those parameters const ... &
ScheduledAction::ScheduledAction(JSObject* _func, List _args, DateTimeMS _nextTime, int _interval, bool _singleShot,
				  int _timerId)
{
  //kDebug(6070) << "ScheduledAction::ScheduledAction(isFunction) " << this;
  func = static_cast<JSObject*>(_func);
  args = _args;
  isFunction = true;
  singleShot = _singleShot;
  nextTime = _nextTime;
  interval = _interval;
  executing = false;
  timerId = _timerId;
}

// KDE 4: Make it const QString &
ScheduledAction::ScheduledAction(QString _code, DateTimeMS _nextTime, int _interval, bool _singleShot, int _timerId)
{
  //kDebug(6070) << "ScheduledAction::ScheduledAction(!isFunction) " << this;
  //func = 0;
  //args = 0;
  func = 0;
  code = _code;
  isFunction = false;
  singleShot = _singleShot;
  nextTime = _nextTime;
  interval = _interval;
  executing = false;
  timerId = _timerId;
}

bool ScheduledAction::execute(Window *window)
{
  KHTMLPart *part = qobject_cast<KHTMLPart*>(window->m_frame->m_part);
  if (!part || !part->jScriptEnabled())
    return false;
  ScriptInterpreter *interpreter = static_cast<ScriptInterpreter *>(part->jScript()->interpreter());

  interpreter->setProcessingTimerCallback(true);

  //kDebug(6070) << "ScheduledAction::execute " << this;
  if (isFunction) {
    if (func->implementsCall()) {
      // #### check this
      Q_ASSERT( part );
      if ( part )
      {
        KJS::Interpreter *interpreter = part->jScript()->interpreter();
        ExecState *exec = interpreter->globalExec();
        Q_ASSERT( window == interpreter->globalObject() );
        JSObject *obj( window );
        func->call(exec,obj,args); // note that call() creates its own execution state for the func call
        if (exec->hadException())
          exec->clearException();

        // Update our document's rendering following the execution of the timeout callback.
        part->document().updateRendering();
      }
    }
  }
  else {
    part->executeScript(DOM::Node(), code);
  }

  interpreter->setProcessingTimerCallback(false);
  return true;
}

void ScheduledAction::mark()
{
  if (func && !func->marked())
    func->mark();
}

ScheduledAction::~ScheduledAction()
{
  args.reset();
  //kDebug(6070) << "ScheduledAction::~ScheduledAction " << this;
}

////////////////////// WindowQObject ////////////////////////

WindowQObject::WindowQObject(Window *w)
  : parent(w)
{
  //kDebug(6070) << "WindowQObject::WindowQObject " << this;
  if ( !parent->m_frame )
      kDebug(6070) << "WARNING: null part in " ;
  else
      connect( parent->m_frame, SIGNAL( destroyed() ),
               this, SLOT( parentDestroyed() ) );
  pauseLevel  = 0;
  lastTimerId = 0;
  currentlyDispatching = false;
}

WindowQObject::~WindowQObject()
{
  //kDebug(6070) << "WindowQObject::~WindowQObject " << this;
  parentDestroyed(); // reuse same code
}

void WindowQObject::parentDestroyed()
{
  killTimers();

  while (!scheduledActions.isEmpty())
    delete scheduledActions.takeFirst();
  scheduledActions.clear();
}

void WindowQObject::pauseTimers()
{
    ++pauseLevel;
    if (pauseLevel == 1)
        pauseStart = DateTimeMS::now();
}

void WindowQObject::resumeTimers()
{
    --pauseLevel;
    if (pauseLevel == 0) {
        // Adjust all timers by the delay length, making sure there is a minimum
        // margin from current time, however, so we don't go stampeding off if
        // there is some unwanted recursion, etc.
        DateTimeMS curTime          = DateTimeMS::now();
        DateTimeMS earliestDispatch = curTime.addMSecs(5);
        int delay = pauseStart.msecsTo(curTime);
        foreach (ScheduledAction *action, scheduledActions) {
            action->nextTime = action->nextTime.addMSecs(delay);
            if (earliestDispatch > action->nextTime)
                action->nextTime = earliestDispatch;
        }

        // Dispatch any timers that may have been ignored if ::timerEvent fell in the middle
        // of a pause..
        timerEvent(0);
    }
}

int WindowQObject::installTimeout(const Identifier &handler, int t, bool singleShot)
{
  int id = ++lastTimerId;
  if (t < 10) t = 10;
  DateTimeMS nextTime = DateTimeMS::now().addMSecs(t);

  ScheduledAction *action = new ScheduledAction(handler.qstring(),nextTime,t,singleShot,id);
  scheduledActions.append(action);
  setNextTimer();
  return id;
}

int WindowQObject::installTimeout(JSValue *func, List args, int t, bool singleShot)
{
  JSObject *objFunc = func->getObject();
  if (!objFunc)
    return 0;
  int id = ++lastTimerId;
  if (t < 10) t = 10;

  DateTimeMS nextTime = DateTimeMS::now().addMSecs(t);
  ScheduledAction *action = new ScheduledAction(objFunc,args,nextTime,t,singleShot,id);
  scheduledActions.append(action);
  setNextTimer();
  return id;
}

void WindowQObject::clearTimeout(int timerId)
{
  foreach (ScheduledAction *action, scheduledActions)
  {
    if (action->timerId == timerId)
    {
      scheduledActions.removeAll(action);
      if (!action->executing)
        delete action;
      return;
    }
  }
}

bool WindowQObject::hasTimers() const
{
  return scheduledActions.count();
}

void WindowQObject::mark()
{
  foreach (ScheduledAction *action, scheduledActions)
  {
    action->mark();
  }
}

void WindowQObject::timerEvent(QTimerEvent *)
{
  killTimers();

  if (scheduledActions.isEmpty())
    return;

  if (pauseLevel)
    return;

  currentlyDispatching = true;


  DateTimeMS current = DateTimeMS::now();

  // Work out which actions are to be executed. We take a separate copy of
  // this list since the main one may be modified during action execution
  QList<ScheduledAction*> toExecute;
  foreach (ScheduledAction *action, scheduledActions)
  {
    if (current >= action->nextTime)
      toExecute.append(action);
  }

  // ### verify that the window can't be closed (and action deleted) during execution
  foreach (ScheduledAction *action, toExecute)
  {
    if (!scheduledActions.count(action)) // removed by clearTimeout()
      continue;

    action->executing = true; // prevent deletion in clearTimeout()

    if (parent->part()) {
      bool ok = action->execute(parent);
      if ( !ok ) // e.g. JS disabled
        scheduledActions.removeAll( action );
    }

    if (action->singleShot)
      scheduledActions.removeAll(action);

    action->executing = false;

    if (!scheduledActions.count(action))
      delete action;
    else
      action->nextTime = action->nextTime.addMSecs(action->interval);
  }

  currentlyDispatching = false;

  // Work out when next event is to occur
  setNextTimer();
}

DateTimeMS DateTimeMS::addMSecs(int s) const
{
  DateTimeMS c = *this;
  c.mTime = mTime.addMSecs(s);
  if (s > 0)
  {
    if (c.mTime < mTime)
      c.mDate = mDate.addDays(1);
  }
  else
  {
    if (c.mTime > mTime)
      c.mDate = mDate.addDays(-1);
  }
  return c;
}

bool DateTimeMS::operator >(const DateTimeMS &other) const
{
  if (mDate > other.mDate)
    return true;

  if (mDate < other.mDate)
    return false;

  return mTime > other.mTime;
}

bool DateTimeMS::operator >=(const DateTimeMS &other) const
{
  if (mDate > other.mDate)
    return true;

  if (mDate < other.mDate)
    return false;

  return mTime >= other.mTime;
}

int DateTimeMS::msecsTo(const DateTimeMS &other) const
{
	int d = mDate.daysTo(other.mDate);
	int ms = mTime.msecsTo(other.mTime);
	return d*24*60*60*1000 + ms;
}


DateTimeMS DateTimeMS::now()
{
  DateTimeMS t;
  QTime before = QTime::currentTime();
  t.mDate = QDate::currentDate();
  t.mTime = QTime::currentTime();
  if (t.mTime < before)
    t.mDate = QDate::currentDate(); // prevent race condition in hacky way :)
  return t;
}

void WindowQObject::setNextTimer()
{
  if (currentlyDispatching)
    return; // Will schedule at the end

  if (scheduledActions.isEmpty())
    return;

  QListIterator<ScheduledAction*> it(scheduledActions);
  DateTimeMS nextTime = it.next()->nextTime;
  while (it.hasNext())
  {
    const DateTimeMS& currTime = it.next()->nextTime;
    if (nextTime > currTime)
      nextTime = currTime;
  }


  int nextInterval = DateTimeMS::now().msecsTo(nextTime);
  if (nextInterval < 0)
    nextInterval = 0;
  timerIds.append(startTimer(nextInterval));
}

void WindowQObject::killTimers()
{
 for (int i = 0; i < timerIds.size(); ++i)
 {
    killTimer(timerIds.at(i));
 }
 timerIds.clear();
}

void WindowQObject::timeoutClose()
{
  parent->closeNow();
}

const ClassInfo FrameArray::info = { "FrameArray", 0, &FrameArrayTable, 0 };

/*
@begin FrameArrayTable 2
length		FrameArray::Length	DontDelete|ReadOnly
location	FrameArray::Location	DontDelete|ReadOnly
@end
*/

JSValue *FrameArray::getValueProperty(ExecState *exec, int token)
{
  switch (token) {
  case Length:
    return jsNumber(part->frames().count());
  case Location:
    // non-standard property, but works in NS and IE
    if (JSObject *obj = Window::retrieveWindow(part))
      return obj->get(exec, "location");
    return jsUndefined();
  default:
    assert(0);
    return jsUndefined();
  }
}

JSValue *FrameArray::indexGetter(ExecState *exec, unsigned index)
{
  KParts::ReadOnlyPart *frame = part->frames().at(index);
  if (frame)
    return Window::retrieve(frame);

  return jsUndefined();
}

JSValue *FrameArray::nameGetter(ExecState *exec, JSObject*, const Identifier& propertyName, const PropertySlot& slot)
{
  FrameArray *thisObj = static_cast<FrameArray *>(slot.slotBase());
  KParts::ReadOnlyPart *frame = thisObj->part->findFrame(propertyName.qstring());
  if (frame)
    return Window::retrieve(frame);
  return jsUndefined();
}

JSValue *FrameArray::nameFallBackGetter(ExecState *exec, JSObject*, const Identifier& propertyName, const PropertySlot& slot)
{
  FrameArray *thisObj = static_cast<FrameArray *>(slot.slotBase());
  DOM::DocumentImpl* doc  = static_cast<DOM::DocumentImpl*>(thisObj->part->document().handle());
  if (doc) {
    DOM::HTMLCollectionImpl docuAll(doc, DOM::HTMLCollectionImpl::DOC_ALL);
    DOM::NodeImpl*     node = docuAll.namedItem(propertyName.domString());
    if (node) {
      if (node->id() == ID_FRAME || node->id() == ID_IFRAME) {
        //Return the Window object.
        KHTMLPart* part = static_cast<DOM::HTMLFrameElementImpl*>(node)->contentPart();
        if (part)
          return Window::retrieveWindow(part);
        else
          return jsUndefined();
      } else {
        //Just a regular node..
        return getDOMNode(exec, node);
      }
    }
  } else {
    kWarning(6070) << "Missing own document in FrameArray::get()";
  }
  return jsUndefined();
}

bool FrameArray::getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot)
{
#ifdef KJS_VERBOSE
  kDebug(6070) << "FrameArray::getOwnPropertySlot " << propertyName.qstring() << " part=" << (void*)part;
#endif

  if (part.isNull()) {
    slot.setUndefined(this);
    return true;
  }

  if (getStaticOwnValueSlot(&FrameArrayTable, this, propertyName, slot))
    return true;

  // check for the name or number
  KParts::ReadOnlyPart *frame = part->findFrame(propertyName.qstring());
  if (frame) {
    slot.setCustom(this, nameGetter);
    return true;
  }

  if (getIndexSlot(this, part->frames().count(), propertyName, slot))
    return true;

  // Fun IE quirk: name lookup in there is actually done by document.all
  // hence, it can find non-frame things (and even let them hide frame ones!)
  // We don't quite do that, but do this as a fallback.
  DOM::DocumentImpl* doc  = static_cast<DOM::DocumentImpl*>(part->document().handle());
  DOM::HTMLCollectionImpl docuAll(doc, DOM::HTMLCollectionImpl::DOC_ALL);
  if (docuAll.namedItem(propertyName.domString())) {
    slot.setCustom(this, nameFallBackGetter);
    return true;
  }

  return JSObject::getOwnPropertySlot(exec, propertyName, slot);
}

UString FrameArray::toString(ExecState *) const
{
  return "[object FrameArray]";
}

JSValue* FrameArray::callAsFunction(ExecState *exec, JSObject * /*thisObj*/, const List &args)
{
    //IE supports a subset of the get functionality as call...
    //... basically, when the return is a window, it supports that, otherwise it
    //errors out. We do a cheap-and-easy emulation of that, and just do the same
    //thing as get does.
    if (args.size() == 1)
        return get(exec, Identifier(args[0]->toString(exec)));

    return jsUndefined();
}


////////////////////// Location Object ////////////////////////

const ClassInfo Location::info = { "Location", 0, &LocationTable, 0 };
/*
@begin LocationTable 11
  hash		Location::Hash		DontDelete
  host		Location::Host		DontDelete
  hostname	Location::Hostname	DontDelete
  href		Location::Href		DontDelete
  pathname	Location::Pathname	DontDelete
  port		Location::Port		DontDelete
  protocol	Location::Protocol	DontDelete
  search	Location::Search	DontDelete
  [[==]]	Location::EqualEqual	DontDelete|ReadOnly
  assign	Location::Assign	DontDelete|Function 1
  toString	Location::ToString	DontDelete|Function 0
  replace	Location::Replace	DontDelete|Function 1
  reload	Location::Reload	DontDelete|Function 0
@end
*/
KJS_IMPLEMENT_PROTOFUNC(LocationFunc)
Location::Location(khtml::ChildFrame *f) : m_frame(f)
{
  //kDebug(6070) << "Location::Location " << this << " m_part=" << (void*)m_part;
}

Location::~Location()
{
  //kDebug(6070) << "Location::~Location " << this << " m_part=" << (void*)m_part;
}

KParts::ReadOnlyPart *Location::part() const {
  return m_frame ? static_cast<KParts::ReadOnlyPart *>(m_frame->m_part) : 0L;
}

bool Location::getOwnPropertySlot(ExecState *exec, const Identifier &p, PropertySlot& slot)
{
#ifdef KJS_VERBOSE
  kDebug(6070) << "Location::getOwnPropertySlot " << p.qstring() << " m_part=" << (void*)m_frame->m_part;
#endif

  if (m_frame.isNull() || m_frame->m_part.isNull())
    return jsUndefined();

  const HashEntry *entry = Lookup::findEntry(&LocationTable, p);

  if ( entry ) {
    // properties that work on all Location objects
    if (entry->value == Replace) {
      getSlotFromEntry<LocationFunc, Location>(entry, this, slot);
      return true;
    }

    // XSS check
    const Window* window = Window::retrieveWindow( m_frame->m_part );
    if ( !window || !window->isSafeScript(exec) ) {
      slot.setUndefined(this);
      return true;
    }

    // XSS check passed - can now dispatch normally.
    getSlotFromEntry<LocationFunc, Location>(entry, this, slot);
    return true;
  }

  return JSObject::getOwnPropertySlot(exec, p, slot);
}

JSValue* Location::getValueProperty(ExecState *exec, int token) const
{
  KUrl url = m_frame->m_part->url();
  switch(token) {
    case Hash:
      return jsString( UString(url.ref().isNull() ? QString("") : '#' + url.ref()) );
    case Host: {
      UString str = url.host();
      if (url.port() > 0)
        str = str + QString(":") + QString::number((int)url.port());
      return jsString(str);
      // Note: this is the IE spec. The NS spec swaps the two, it says
      // "The hostname property is the concatenation of the host and port properties, separated by a colon."
      // Bleh.
    }
    case Hostname:
      return jsString( UString(url.host()) );
    case Href:
      if (url.isEmpty())
	return jsString("about:blank");
      else if (!url.hasPath())
        return jsString( UString(url.prettyUrl()+'/') );
      else
        return jsString( UString(url.prettyUrl()) );
    case Pathname:
      if (url.isEmpty())
	return jsString("");
      return jsString( UString(url.path().isEmpty() ? QString("/") : url.path()) );
    case Port:
      return jsString( UString(url.port() ? QString::number((int)url.port()) : QLatin1String("")) );
    case Protocol:
      return jsString( UString(url.protocol()+':') );
    case Search:
      return jsString( UString(url.query()) );
    case EqualEqual: // [[==]]
      return jsString(toString(exec));
  }
  return jsUndefined();
}

void Location::put(ExecState *exec, const Identifier &p, JSValue *v, int attr)
{
#ifdef KJS_VERBOSE
  kDebug(6070) << "Location::put " << p.qstring() << " m_part=" << (void*)m_frame->m_part;
#endif
  if (m_frame.isNull() || m_frame->m_part.isNull())
    return;

  const Window* window = Window::retrieveWindow( m_frame->m_part );
  if ( !window )
    return;

  KUrl url = m_frame->m_part->url();

  const HashEntry *entry = Lookup::findEntry(&LocationTable, p);

  if (entry) {

    // XSS check. Only new hrefs can be set from other sites
    if (entry->value != Href && !window->isSafeScript(exec))
      return;

    QString str = v->toString(exec).qstring();
    switch (entry->value) {
    case Href: {
      KHTMLPart* p =qobject_cast<KHTMLPart*>(Window::retrieveActive(exec)->part());
      if ( p )
        url = p->htmlDocument().completeURL( str ).string();
      else
        url = str;
      break;
    }
    case Hash:
      // Strip any leading # --- setting hash to #foo is the same as setting it to foo.
      if (str.startsWith(QLatin1Char('#')))
        str = str.mid(1);

      // Note that we want to do gotoAnchor even when the hash is already set, so we
      // scroll the destination into view
      url.setRef(str);
      break;
    case Host: {
      QString host = str.left(str.indexOf(":"));
      QString port = str.mid(str.indexOf(":")+1);
      url.setHost(host);
      url.setPort(port.toUInt());
      break;
    }
    case Hostname:
      url.setHost(str);
      break;
    case Pathname:
      url.setPath(str);
      break;
    case Port:
      url.setPort(str.toUInt());
      break;
    case Protocol:
      url.setProtocol(str);
      break;
    case Search:
      url.setQuery(str);
      break;
    }
  } else {
    JSObject::put(exec, p, v, attr);
    return;
  }

  Window::retrieveWindow(m_frame->m_part)->goURL(exec, url.url(), false /* don't lock history*/ );
}

JSValue *Location::toPrimitive(ExecState *exec, JSType) const
{
  if (m_frame) {
    Window* window = Window::retrieveWindow( m_frame->m_part );
    if ( window && window->isSafeScript(exec) )
      return jsString(toString(exec));
  }
  return jsUndefined();
}

UString Location::toString(ExecState *exec) const
{
  if (m_frame) {
    Window* window = Window::retrieveWindow( m_frame->m_part );
    if ( window && window->isSafeScript(exec) )
    {
      KUrl url = m_frame->m_part->url();
      if (url.isEmpty())
        return "about:blank";
      else if (!url.hasPath())
        return url.prettyUrl()+'/';
      else
        return url.prettyUrl();
    }
  }
  return "";
}

JSValue *LocationFunc::callAsFunction(ExecState *exec, JSObject *thisObj, const List &args)
{
  KJS_CHECK_THIS( Location, thisObj );
  Location *location = static_cast<Location *>(thisObj);
  KParts::ReadOnlyPart *part = location->part();

  if (!part) return jsUndefined();

  Window* window = Window::retrieveWindow(part);

  if ( !window->isSafeScript(exec) && id != Location::Replace)
      return jsUndefined();

  switch (id) {
  case Location::Assign:
  case Location::Replace:
    Window::retrieveWindow(part)->goURL(exec, args[0]->toString(exec).qstring(),
            id == Location::Replace);
    break;
  case Location::Reload: {
    KHTMLPart *khtmlpart = qobject_cast<KHTMLPart*>(part);
    if (khtmlpart)
      khtmlpart->scheduleRedirection(-1, part->url().url(), true/*lock history*/);
    else
      part->openUrl(part->url());
    break;
  }
  case Location::ToString:
    return jsString(location->toString(exec));
  }
  return jsUndefined();
}

////////////////////// External Object ////////////////////////

const ClassInfo External::info = { "External", 0, 0, 0 };
/*
@begin ExternalTable 4
  addFavorite	External::AddFavorite	DontDelete|Function 1
@end
*/
KJS_IMPLEMENT_PROTOFUNC(ExternalFunc)

bool External::getOwnPropertySlot(ExecState *exec, const Identifier &p, PropertySlot& propertySlot)
{
  return getStaticFunctionSlot<ExternalFunc,JSObject>(exec, &ExternalTable, this, p, propertySlot);
}

JSValue *ExternalFunc::callAsFunction(ExecState *exec, JSObject *thisObj, const List &args)
{
  KJS_CHECK_THIS( External, thisObj );
  External *external = static_cast<External *>(thisObj);

  KHTMLPart *part = external->part;
  if (!part)
    return jsUndefined();

  KHTMLView *widget = part->view();

  switch (id) {
  case External::AddFavorite:
  {
#ifndef KONQ_EMBEDDED
    if (!widget->dialogsAllowed())
      return jsUndefined();
    part->xmlDocImpl()->updateRendering();
    if (args.size() != 1 && args.size() != 2)
      return jsUndefined();

    QString url = args[0]->toString(exec).qstring();
    QString title;
    if (args.size() == 2)
      title = args[1]->toString(exec).qstring();

    // AK - don't do anything yet, for the moment i
    // just wanted the base js handling code in cvs
    return jsUndefined();

    QString question;
    if ( title.isEmpty() )
      question = i18n("Do you want a bookmark pointing to the location \"%1\" to be added to your collection?",
                  url);
    else
      question = i18n("Do you want a bookmark pointing to the location \"%1\" titled \"%2\" to be added to your collection?",
                  url, title);

    emit part->browserExtension()->requestFocus(part);

    QString caption;
    if (!part->url().host().isEmpty())
       caption = part->url().host() + " - ";
    caption += i18n("JavaScript Attempted Bookmark Insert");

    if (KMessageBox::warningYesNo(
          widget, question, caption,
          KGuiItem(i18n("Insert")), KGuiItem(i18n("Disallow"))) == KMessageBox::Yes)
    {
      KBookmarkManager *mgr = KBookmarkManager::userBookmarksManager();
      KBookmarkDialog dlg(mgr, 0);
      dlg.addBookmark(title, url);
    }
#else
    return jsUndefined();
#endif
    break;
  }
  default:
    return jsUndefined();
  }

  return jsUndefined();
}

////////////////////// History Object ////////////////////////

const ClassInfo History::info = { "History", 0, 0, 0 };
/*
@begin HistoryTable 4
  length	History::Length		DontDelete|ReadOnly
  back		History::Back		DontDelete|Function 0
  forward	History::Forward	DontDelete|Function 0
  go		History::Go		DontDelete|Function 1
@end
*/
KJS_IMPLEMENT_PROTOFUNC(HistoryFunc)

bool History::getOwnPropertySlot(ExecState *exec, const Identifier &p, PropertySlot& slot)
{
  return getStaticPropertySlot<HistoryFunc,History,JSObject>(exec, &HistoryTable, this, p, slot);
}

JSValue *History::getValueProperty(ExecState *, int token) const
{
  // if previous or next is implemented, make sure its not a major
  // privacy leak (see i.e. http://security.greymagic.com/adv/gm005-op/)
  switch (token) {
  case Length:
  {
    if ( !part )
      return jsNumber( 0 );

    KParts::BrowserExtension *ext = part->browserExtension();
    if ( !ext )
      return jsNumber( 0 );

    KParts::BrowserInterface *iface = ext->browserInterface();
    if ( !iface )
      return jsNumber( 0 );

    QVariant length = iface->property( "historyLength" );

    if ( length.type() != QVariant::UInt )
      return jsNumber( 0 );

    return jsNumber( length.toUInt() );
  }
  default:
    kDebug(6070) << "WARNING: Unhandled token in History::getValueProperty : " << token;
    return jsUndefined();
  }
}

JSValue *HistoryFunc::callAsFunction(ExecState *exec, JSObject *thisObj, const List &args)
{
  KJS_CHECK_THIS( History, thisObj );
  History *history = static_cast<History *>(thisObj);

  JSValue *v = args[0];
  double n = 0.0;
  if(v)
    n = v->toInteger(exec);

  int steps;
  switch (id) {
  case History::Back:
    steps = -1;
    break;
  case History::Forward:
    steps = 1;
    break;
  case History::Go:
    steps = (int)n;
    break;
  default:
    return jsUndefined();
  }

  // Special case for go(0) from a frame -> reload only the frame
  // go(i!=0) from a frame navigates into the history of the frame only,
  // in both IE and NS (but not in Mozilla).... we can't easily do that
  // in Konqueror...
  if (!steps) // add && history->part->parentPart() to get only frames, but doesn't matter
  {
    history->part->openUrl( history->part->url() ); /// ## need args.reload=true?
  } else
  {
    // Delay it.
    // Testcase: history.back(); alert("hello");
    Window* window = Window::retrieveWindow( history->part );
    window->delayedGoHistory( steps );
  }
  return jsUndefined();
}

} // namespace KJS


#include "kjs_window.moc"
