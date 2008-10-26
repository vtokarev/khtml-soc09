/* This file is part of the KDE project
   Copyright (C) 2001 Andrea Rizzi <rizzi@kde.org>
	              Ulrich Kuettler <ulrich.kuettler@mailbox.tu-dresden.de>
   Copyright (C) 2006 Martin Pfeiffer <hubipete@gmx.net>
   Copyright (C) 2006 Alfredo Beaumont Sainz <alfredo.beaumont@gmail.com>

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

#include "BasicElement.h"
#include "AttributeManager.h"
#include <QXmlStreamWriter>
#include <QDomElement>
#include <QPainter>
#include <QVariant>

#include <kdebug.h>

BasicElement::BasicElement( BasicElement* p ) : m_parentElement( p )
{
    m_scaleFactor = 1.0;
    m_scaleLevel = 1;
    m_boundingRect.setTopLeft( QPointF( 0.0, 0.0 ) );
    m_boundingRect.setWidth( 7.0 );       // standard values
    m_boundingRect.setHeight( 10.0 );
    m_displayStyle = true;
    setBaseLine( 10.0 );

}

BasicElement::~BasicElement()
{
    m_attributes.clear();
}

void BasicElement::paint( QPainter& painter, AttributeManager* )
{ 
/*    painter.setPen( QPen( Qt::blue ) );
    painter.drawRect( QRectF(0.0, 0.0, width(), height()) );
    painter.setPen( QPen( Qt::red, 0, Qt::DashLine ) );
    painter.drawLine( QPointF(0.0, baseLine()), QPointF(width(), baseLine()));*/
}

void BasicElement::layout( const AttributeManager* )
{ /* do nothing */ }

BasicElement* BasicElement::acceptCursor( const FormulaCursor* cursor )
{
    Q_UNUSED( cursor )
    return 0;
}

void BasicElement::insertChild( FormulaCursor* cursor, BasicElement* element )
{
    // call the parentElement to notify it that there is something to be inserted
    m_parentElement->insertChild( cursor, element );
}

void BasicElement::removeChild( FormulaCursor*, BasicElement* )
{ /* do nothing a BasicElement has no children */ }

const QList<BasicElement*> BasicElement::childElements()
{
    kWarning( 39001) << "Returning no elements from BasicElement";
    return QList<BasicElement*>();
}

BasicElement* BasicElement::childElementAt( const QPointF& p )
{
    if( !m_boundingRect.contains( p ) )
        return 0;

    if( childElements().isEmpty() )
        return this;

    BasicElement* ownerElement = 0;
    foreach( BasicElement* tmpElement, childElements() )
    {
        ownerElement = tmpElement->childElementAt( p );

        if( ownerElement )
            return ownerElement;
    }

    return this;    // if no child contains the point, it's the FormulaElement itsself
}

void BasicElement::setAttribute( const QString& name, const QVariant& value )
{
    if( name.isEmpty() || !value.canConvert( QVariant::String ) )
        return;

    if( value.isNull() )
        m_attributes.remove( name );
    else
        m_attributes.insert( name, value.toString() );
}

QString BasicElement::attribute( const QString& attribute ) const
{
    QString tmp = m_attributes.value( attribute );
    if( tmp.isEmpty() )
        return QString();

    return tmp;
}

QString BasicElement::inheritsAttribute( const QString& ) const
{
    return QString();   // do nothing
}

QString BasicElement::attributesDefaultValue( const QString& ) const
{
    return QString();  // do nothing
}

bool BasicElement::readMathML( const QDomElement& element )
{
    readMathMLAttributes( element );
    return readMathMLContent( element );
}

void BasicElement::readMathMLAttributes( const QDomElement& element )
{
    QDomNamedNodeMap al = element.attributes();
    for(int i = 0; i < al.count(); ++i) {
        QDomAttr a = al.item(i).toAttr();
        m_attributes.insert( a.name().toLower(),
                             a.value().toLower() );
    }
}

bool BasicElement::readMathMLContent( const QDomElement& parent )
{
    Q_UNUSED( parent )
    return true;
}
void BasicElement::writeMathML( QXmlStreamWriter* writer ) const
{
    if( elementType() == Basic )
        return;

    const QByteArray name = ElementFactory::elementName( elementType() ).toLatin1();
    writer->writeStartElement( name );
    writeMathMLAttributes( writer );
    writeMathMLContent( writer );
    writer->writeEndElement();
}

void BasicElement::writeMathMLAttributes( QXmlStreamWriter* writer ) const
{
    foreach( const QString &value, m_attributes )
        writer->writeAttribute( m_attributes.key( value ), value );
}

void BasicElement::writeMathMLContent( QXmlStreamWriter* writer ) const
{
    Q_UNUSED( writer )   // this is just to be reimplemented
}
ElementType BasicElement::elementType() const
{
    return Basic;
}

const QRectF& BasicElement::boundingRect() const
{
    return m_boundingRect;
}

double BasicElement::height() const
{
    return m_boundingRect.height();
}

double BasicElement::width() const
{
    return m_boundingRect.width();
}

double BasicElement::baseLine() const
{
    return m_baseLine;
}

QPointF BasicElement::origin() const
{
    return m_boundingRect.topLeft();
}

BasicElement* BasicElement::parentElement() const
{
    return m_parentElement;
}

double BasicElement::scaleFactor() const
{
    return m_scaleFactor;
}
int BasicElement::scaleLevel() const
{
    return m_scaleLevel;
}

void BasicElement::setWidth( double width )
{
    m_boundingRect.setWidth( width );
}

void BasicElement::setHeight( double height )
{
    m_boundingRect.setHeight( height );
}

void BasicElement::setOrigin( QPointF origin )
{
    m_boundingRect.moveTopLeft( origin );
}

void BasicElement::setBaseLine( double baseLine )
{
    m_baseLine = baseLine;
}

void BasicElement::setParentElement( BasicElement* parent )
{
    m_parentElement = parent;
}

void BasicElement::setScaleLevel( int scaleLevel )
{
    if(scaleLevel == m_scaleLevel) return;

    m_scaleLevel =  qMax(scaleLevel, 0);
    int level = scaleLevel;
    m_scaleFactor = 1.9;
    while(level-- > 0)  //raise multiplier to the power of level
        m_scaleFactor *= 0.71;
}

bool BasicElement::displayStyle() const
{
    return m_displayStyle;
}
void BasicElement::setDisplayStyle(bool displayStyle)
{
    m_displayStyle = displayStyle;
}
