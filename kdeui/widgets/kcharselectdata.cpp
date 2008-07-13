/* This file is part of the KDE libraries

   Copyright (C) 2007 Daniel Laidig <d.laidig@gmx.de>

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

#include "kcharselectdata_p.h"

#include <QStringList>
#include <QFile>
#include <qendian.h>

#include <string.h>
#include <klocalizedstring.h>
#include <kstandarddirs.h>

/* constants for hangul (de)composition, see UAX #15 */
#define SBase 0xAC00
#define LBase 0x1100
#define VBase 0x1161
#define TBase 0x11A7
#define LCount 19
#define VCount 21
#define TCount 28
#define NCount (VCount * TCount)
#define SCount (LCount * NCount)

static const char JAMO_L_TABLE[][4] =
    {
        "G", "GG", "N", "D", "DD", "R", "M", "B", "BB",
        "S", "SS", "", "J", "JJ", "C", "K", "T", "P", "H"
    };

static const char JAMO_V_TABLE[][4] =
    {
        "A", "AE", "YA", "YAE", "EO", "E", "YEO", "YE", "O",
        "WA", "WAE", "OE", "YO", "U", "WEO", "WE", "WI",
        "YU", "EU", "YI", "I"
    };

static const char JAMO_T_TABLE[][4] =
    {
        "", "G", "GG", "GS", "N", "NJ", "NH", "D", "L", "LG", "LM",
        "LB", "LS", "LT", "LP", "LH", "M", "B", "BS",
        "S", "SS", "NG", "J", "C", "K", "T", "P", "H"
    };

bool KCharSelectData::openDataFile()
{
    if(!dataFile.isEmpty()) {
        return true;
    } else {
        QFile file(KStandardDirs::locate("data", "kcharselect/kcharselect-data"));
        if (!file.open(QIODevice::ReadOnly)) {
            return false;
        }
        dataFile = file.readAll();
        file.close();
        return true;
    }
}

quint32 KCharSelectData::getDetailIndex(const QChar& c) const
{
    const uchar* data = reinterpret_cast<const uchar*>(dataFile.constData());
    // Convert from little-endian, so that this code works on PPC too.
    // http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=482286
    const quint32 offsetBegin = qFromLittleEndian<quint32>(data+12);
    const quint32 offsetEnd = qFromLittleEndian<quint32>(data+16);

    int min = 0;
    int mid;
    int max = ((offsetEnd - offsetBegin) / 27) - 1;

    quint16 unicode = c.unicode();

    static quint16 most_recent_searched;
    static quint32 most_recent_result;


    if (unicode == most_recent_searched)
        return most_recent_result;

    most_recent_searched = unicode;

    while (max >= min) {
        mid = (min + max) / 2;
        const quint16 midUnicode = qFromLittleEndian<quint16>(data + offsetBegin + mid*27);
        if (unicode > midUnicode)
            min = mid + 1;
        else if (unicode < midUnicode)
            max = mid - 1;
        else {
            most_recent_result = offsetBegin + mid*27;

            return most_recent_result;
        }
    }

    most_recent_result = 0;
    return 0;
}

QString KCharSelectData::formatCode(ushort code, int length, const QString& prefix, int base)
{
    QString s = QString::number(code, base).toUpper();
    while (s.size() < length)
        s.prepend('0');
    s.prepend(prefix);
    return s;
}

QList<QChar> KCharSelectData::blockContents(int block)
{
    if(!openDataFile()) {
        return QList<QChar>();
    }

    const uchar* data = reinterpret_cast<const uchar*>(dataFile.constData());
    const quint32 offsetBegin = qFromLittleEndian<quint32>(data+20);
    const quint32 offsetEnd = qFromLittleEndian<quint32>(data+24);

    int max = ((offsetEnd - offsetBegin) / 4) - 1;

    QList<QChar> res;

    if(block > max)
        return res;

    quint16 unicodeBegin = qFromLittleEndian<quint16>(data + offsetBegin + block*4);
    quint16 unicodeEnd = qFromLittleEndian<quint16>(data + offsetBegin + block*4 + 2);

    while(unicodeBegin < unicodeEnd) {
        res.append(unicodeBegin);
        unicodeBegin++;
    }
    res.append(unicodeBegin); // Be carefull when unicodeEnd==0xffff

    return res;
}

QList<int> KCharSelectData::sectionContents(int section)
{
    if(!openDataFile()) {
        return QList<int>();
    }

    const uchar* data = reinterpret_cast<const uchar*>(dataFile.constData());
    const quint32 offsetBegin = qFromLittleEndian<quint32>(data+28);
    const quint32 offsetEnd = qFromLittleEndian<quint32>(data+32);

    int max = ((offsetEnd - offsetBegin) / 4) - 1;

    QList<int> res;

    if(section > max)
        return res;

    for(int i = 0; i <= max; i++) {
        const quint16 currSection = qFromLittleEndian<quint16>(data + offsetBegin + i*4);
        if(currSection == section) {
            res.append( qFromLittleEndian<quint16>(data + offsetBegin + i*4 + 2) );
        }
    }

    return res;
}

QStringList KCharSelectData::sectionList()
{
    if(!openDataFile()) {
        return QStringList();
    }

    const uchar* udata = reinterpret_cast<const uchar*>(dataFile.constData());
    const quint32 stringBegin = qFromLittleEndian<quint32>(udata+24);
    const quint32 stringEnd = qFromLittleEndian<quint32>(udata+28);

    const char* data = dataFile.constData();
    QStringList list;
    quint32 i = stringBegin;
    while(i < stringEnd) {
        list.append(i18nc("KCharSelect section name", data + i));
        i += strlen(data + i) + 1;
    }

    return list;
}

QString KCharSelectData::block(const QChar& c)
{
    return blockName(blockIndex(c));
}

QString KCharSelectData::name(const QChar& c)
{
    if(!openDataFile()) {
        return QString();
    }

    ushort unicode = c.unicode();
    if ((unicode >= 0x3400 && unicode <= 0x4DB5)
            || (unicode >= 0x4e00 && unicode <= 0x9fa5)) {
        // || (unicode >= 0x20000 && unicode <= 0x2A6D6) // useless, since limited to 16 bit
        return "CJK UNIFIED IDEOGRAPH-" + QString::number(unicode, 16);
    } else if (c >= 0xac00 && c <= 0xd7af) {
        /* compute hangul syllable name as per UAX #15 */
        int SIndex = c.unicode() - SBase;
        int LIndex, VIndex, TIndex;

        if (SIndex < 0 || SIndex >= SCount)
            return QString();

        LIndex = SIndex / NCount;
        VIndex = (SIndex % NCount) / TCount;
        TIndex = SIndex % TCount;

        return QString("HANGUL SYLLABLE ") + JAMO_L_TABLE[LIndex] + JAMO_V_TABLE[VIndex] + JAMO_T_TABLE[TIndex];
    } else if (unicode >= 0xD800 && unicode <= 0xDB7F)
        return i18n("<Non Private Use High Surrogate>");
    else if (unicode >= 0xDB80 && unicode <= 0xDBFF)
        return i18n("<Private Use High Surrogate>");
    else if (unicode >= 0xDC00 && unicode <= 0xDFFF)
        return i18n("<Low Surrogate>");
    else if (unicode >= 0xE000 && unicode <= 0xF8FF)
        return i18n("<Private Use>");
//  else if (unicode >= 0xF0000 && unicode <= 0xFFFFD) // 16 bit!
//   return i18n("<Plane 15 Private Use>");
//  else if (unicode >= 0x100000 && unicode <= 0x10FFFD)
//   return i18n("<Plane 16 Private Use>");
    else {
        const uchar* data = reinterpret_cast<const uchar*>(dataFile.constData());
        const quint32 offsetBegin = qFromLittleEndian<quint32>(data+4);
        const quint32 offsetEnd = qFromLittleEndian<quint32>(data+8);

        int min = 0;
        int mid;
        int max = ((offsetEnd - offsetBegin) / 6) - 1;
        QString s;

        while (max >= min) {
            mid = (min + max) / 2;
            const quint16 midUnicode = qFromLittleEndian<quint16>(data + offsetBegin + mid*6);
            if (unicode > midUnicode)
                min = mid + 1;
            else if (unicode < midUnicode)
                max = mid - 1;
            else {
                quint32 offset = qFromLittleEndian<quint32>(data + offsetBegin + mid*6 + 2);
                s = QString(dataFile.constData() + offset);
                break;
            }
        }

        if (s.isNull()) {
            return i18n("<not assigned>");
        } else {
            return s;
        }
    }
}

int KCharSelectData::blockIndex(const QChar& c)
{
    if(!openDataFile()) {
        return 0;
    }

    const uchar* data = reinterpret_cast<const uchar*>(dataFile.constData());
    const quint32 offsetBegin = qFromLittleEndian<quint32>(data+20);
    const quint32 offsetEnd = qFromLittleEndian<quint32>(data+24);
    const quint16 unicode = c.unicode();

    int max = ((offsetEnd - offsetBegin) / 4) - 1;

    int i = 0;

    while (unicode > qFromLittleEndian<quint16>(data + offsetBegin + i*4 + 2) && i < max) {
        i++;
    }

    return i;
}

int KCharSelectData::sectionIndex(int block)
{
    if(!openDataFile()) {
        return 0;
    }

    const uchar* data = reinterpret_cast<const uchar*>(dataFile.constData());
    const quint32 offsetBegin = qFromLittleEndian<quint32>(data+28);
    const quint32 offsetEnd = qFromLittleEndian<quint32>(data+32);

    int max = ((offsetEnd - offsetBegin) / 4) - 1;

    for(int i = 0; i <= max; i++) {
        if( qFromLittleEndian<quint16>(data + offsetBegin + i*4 + 2) == block) {
            return qFromLittleEndian<quint16>(data + offsetBegin + i*4);
        }
    }

    return 0;
}

QString KCharSelectData::blockName(int index)
{
    if(!openDataFile()) {
        return QString();
    }

    const uchar* udata = reinterpret_cast<const uchar*>(dataFile.constData());
    const quint32 stringBegin = qFromLittleEndian<quint32>(udata+16);
    const quint32 stringEnd = qFromLittleEndian<quint32>(udata+20);

    quint32 i = stringBegin;
    int currIndex = 0;

    const char* data = dataFile.constData();
    while(i < stringEnd && currIndex < index) {
        i += strlen(data + i) + 1;
        currIndex++;
    }

    return i18nc("KCharselect unicode block name", data + i);
}

QStringList KCharSelectData::aliases(const QChar& c)
{
    if(!openDataFile()) {
        return QStringList();
    }
    const uchar* udata = reinterpret_cast<const uchar*>(dataFile.constData());
    const int detailIndex = getDetailIndex(c);
    if(detailIndex == 0) {
        return QStringList();
    }

    const quint8 count = * (quint8 *)(udata + detailIndex + 6);
    quint32 offset = qFromLittleEndian<quint32>(udata + detailIndex + 2);

    QStringList aliases;

    const char* data = dataFile.constData();
    for (int i = 0;  i < count;  i++) {
        aliases.append(QString::fromUtf8(data + offset));
        offset += strlen(data + offset) + 1;
    }
    return aliases;
}

QStringList KCharSelectData::notes(const QChar& c)
{
    if(!openDataFile()) {
        return QStringList();
    }
    const int detailIndex = getDetailIndex(c);
    if(detailIndex == 0) {
        return QStringList();
    }

    const uchar* udata = reinterpret_cast<const uchar*>(dataFile.constData());
    const quint8 count = * (quint8 *)(udata + detailIndex + 11);
    quint32 offset = qFromLittleEndian<quint32>(udata + detailIndex + 7);

    QStringList notes;

    const char* data = dataFile.constData();
    for (int i = 0;  i < count;  i++) {
        notes.append(QString::fromLatin1(data + offset));
        offset += strlen(data + offset) + 1;
    }

    return notes;
}

QList<QChar> KCharSelectData::seeAlso(const QChar& c)
{
    if(!openDataFile()) {
        return QList<QChar>();
    }
    const int detailIndex = getDetailIndex(c);
    if(detailIndex == 0) {
        return QList<QChar>();
    }

    const uchar* udata = reinterpret_cast<const uchar*>(dataFile.constData());
    const quint8 count = * (quint8 *)(udata + detailIndex + 26);
    quint32 offset = qFromLittleEndian<quint32>(udata + detailIndex + 22);

    QList<QChar> seeAlso;

    for (int i = 0;  i < count;  i++) {
        seeAlso.append(qFromLittleEndian<quint16> (udata + offset));
        offset += 2;
    }

    return seeAlso;
}

QStringList KCharSelectData::equivalents(const QChar& c)
{
    if(!openDataFile()) {
        return QStringList();
    }
    const int detailIndex = getDetailIndex(c);
    if(detailIndex == 0) {
        return QStringList();
    }

    const uchar* udata = reinterpret_cast<const uchar*>(dataFile.constData());
    const quint8 count = * (quint8 *)(udata + detailIndex + 21);
    quint32 offset = qFromLittleEndian<quint32>(udata + detailIndex + 17);

    QStringList equivalents;

    const char* data = dataFile.constData();
    for (int i = 0;  i < count;  i++) {
        equivalents.append(QString::fromUtf8(data + offset));
        offset += strlen(data + offset) + 1;
    }

    return equivalents;
}

QStringList KCharSelectData::approximateEquivalents(const QChar& c)
{
    if(!openDataFile()) {
        return QStringList();
    }
    const int detailIndex = getDetailIndex(c);
    if(detailIndex == 0) {
        return QStringList();
    }

    const uchar* udata = reinterpret_cast<const uchar*>(dataFile.constData());
    const quint8 count = * (quint8 *)(udata + detailIndex + 16);
    quint32 offset = qFromLittleEndian<quint32>(udata + detailIndex + 12);

    QStringList approxEquivalents;

    const char* data = dataFile.constData();
    for (int i = 0;  i < count;  i++) {
        approxEquivalents.append(QString::fromUtf8(data + offset));
        offset += strlen(data + offset) + 1;
    }

    return approxEquivalents;
}

QStringList KCharSelectData::unihanInfo(const QChar& c)
{
    if(!openDataFile()) {
        return QStringList();
    }

    const char* data = dataFile.constData();
    const uchar* udata = reinterpret_cast<const uchar*>(data);
    const quint32 offsetBegin = qFromLittleEndian<quint32>(udata+36);
    const quint32 offsetEnd = dataFile.size();

    int min = 0;
    int mid;
    int max = ((offsetEnd - offsetBegin) / 30) - 1;
    quint16 unicode = c.unicode();

    while (max >= min) {
        mid = (min + max) / 2;
        const quint16 midUnicode = qFromLittleEndian<quint16>(udata + offsetBegin + mid*30);
        if (unicode > midUnicode)
            min = mid + 1;
        else if (unicode < midUnicode)
            max = mid - 1;
        else {
            QStringList res;
            for(int i = 0; i < 7; i++) {
                quint32 offset = qFromLittleEndian<quint32>(udata + offsetBegin + mid*30 + 2 + i*4);
                if(offset != 0) {
                    res.append(QString::fromUtf8(data + offset));
                } else {
                    res.append(QString());
                }
            }
            return res;
        }
    }

    return QStringList();
}

QString KCharSelectData::categoryText(QChar::Category category)
{
    switch (category) {
    case QChar::Other_Control: return i18n("Other, Control");
    case QChar::Other_Format: return i18n("Other, Format");
    case QChar::Other_NotAssigned: return i18n("Other, Not Assigned");
    case QChar::Other_PrivateUse: return i18n("Other, Private Use");
    case QChar::Other_Surrogate: return i18n("Other, Surrogate");
    case QChar::Letter_Lowercase: return i18n("Letter, Lowercase");
    case QChar::Letter_Modifier: return i18n("Letter, Modifier");
    case QChar::Letter_Other: return i18n("Letter, Other");
    case QChar::Letter_Titlecase: return i18n("Letter, Titlecase");
    case QChar::Letter_Uppercase: return i18n("Letter, Uppercase");
    case QChar::Mark_SpacingCombining: return i18n("Mark, Spacing Combining");
    case QChar::Mark_Enclosing: return i18n("Mark, Enclosing");
    case QChar::Mark_NonSpacing: return i18n("Mark, Non-Spacing");
    case QChar::Number_DecimalDigit: return i18n("Number, Decimal Digit");
    case QChar::Number_Letter: return i18n("Number, Letter");
    case QChar::Number_Other: return i18n("Number, Other");
    case QChar::Punctuation_Connector: return i18n("Punctuation, Connector");
    case QChar::Punctuation_Dash: return i18n("Punctuation, Dash");
    case QChar::Punctuation_Close: return i18n("Punctuation, Close");
    case QChar::Punctuation_FinalQuote: return i18n("Punctuation, Final Quote");
    case QChar::Punctuation_InitialQuote: return i18n("Punctuation, Initial Quote");
    case QChar::Punctuation_Other: return i18n("Punctuation, Other");
    case QChar::Punctuation_Open: return i18n("Punctuation, Open");
    case QChar::Symbol_Currency: return i18n("Symbol, Currency");
    case QChar::Symbol_Modifier: return i18n("Symbol, Modifier");
    case QChar::Symbol_Math: return i18n("Symbol, Math");
    case QChar::Symbol_Other: return i18n("Symbol, Other");
    case QChar::Separator_Line: return i18n("Separator, Line");
    case QChar::Separator_Paragraph: return i18n("Separator, Paragraph");
    case QChar::Separator_Space: return i18n("Separator, Space");
    default: return i18n("Unknown");
    }
}

QList<QChar> KCharSelectData::find(const QString& needle)
{
    QList<QChar> res;
    QStringList searchStrings = needle.simplified().split(' ');

    if (searchStrings.count() == 0) {
        return res;
    }

    if(searchStrings.count() == 1 && searchStrings[0].length() == 1) {
        res.append(searchStrings[0].at(0));
        return res;
    }

    QRegExp regExp("^(|u\\+|U\\+|0x|0X)([A-Fa-f0-9]{4})$");
    foreach(const QString &s, searchStrings) {
        if(regExp.exactMatch(s)) {
            res.append(regExp.cap(2).toInt(0, 16));
        }
    }

    QString firstString = searchStrings.takeFirst();

    const char* data = dataFile.constData();
    const uchar* udata = reinterpret_cast<const uchar*>(data);
    const quint32 offsetBegin = qFromLittleEndian<quint32>(udata+4);
    const quint32 offsetEnd = qFromLittleEndian<quint32>(udata+8);

    int max = ((offsetEnd - offsetBegin) / 6) - 1;

    for (int i = 0; i <= max; i++) {
        quint32 offset = qFromLittleEndian<quint32>(udata + offsetBegin + i*6 + 2);
        QString name(data + offset);
        if (name.contains(firstString, Qt::CaseInsensitive)) {
            bool valid = true;
            foreach(const QString &s, searchStrings) {
                if (!name.contains(s, Qt::CaseInsensitive)) {
                    valid = false;
                    break;
                }
            }
            if (valid) {
                res.append(qFromLittleEndian<quint16>(udata + offsetBegin + i*6));
            }
        }
    }
    return res;
}
