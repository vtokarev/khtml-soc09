/*  This file is part of the KDE libraries    Copyright (C) 2007 Chusslove Illich <caslav.ilic@gmx.net>

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

#include <ktranslit_p.h>
#include <kdebug.h>

#include <config.h>

#include <QHash>

// -----------------------------------------------------------------------------
// Base class.

class KTranslitPrivate
{
};

KTranslit::KTranslit ()
: d(NULL)
{
}

KTranslit::~KTranslit ()
{
    delete d;
}

KTranslit *KTranslit::create (const QString &lang)
{
    if (lang == QString::fromAscii("sr")) {
        return new KTranslitSerbian();
    }
    else {
        return NULL;
    }
}

QStringList KTranslit::fallbackList (const QString &lang)
{
    QStringList fallbacks;

    if (   lang == QString::fromAscii("sr@latin")
        || lang == QString::fromAscii("sr@Latn")) {
        fallbacks += QString::fromAscii("sr");
    }

    return fallbacks;
}

void splitLangScript (const QString &lang, QString &ln, QString &scr)
{
    ln = lang;
    scr.clear();
    int pos = lang.indexOf('@');
    if (pos >= 0) {
        ln = lang.left(pos);
        scr = lang.mid(pos + 1);
    }
}

QString KTranslit::higherPriorityScript (const QString &lang,
                                         const KLocale *locale)
{
    if (locale == NULL) {
        return QString();
    }

    // Split into pure language and script part.
    QString ln, scr;
    splitLangScript(lang, ln, scr);

    // Search through higher priority languages.
    QString finalScrHi;
    if (lang != KLocale::defaultLanguage()) {
        foreach (const QString &langHi, locale->languageList()) {
            // Don't search lower priority languages.
            if (langHi == lang)
                break;

            // Split current spec into pure language and script parts.
            QString lnHi, scrHi;
            splitLangScript(langHi, lnHi, scrHi);

            // Return current script if languages match.
            if (lnHi == ln) {
                finalScrHi = scrHi;
                break;
            }
        }
    }
    return finalScrHi;
}

QString KTranslit::transliterate (const QString &str,
                                  const QString &script) const
{
    Q_UNUSED(script);
    return str;
}

// Head of alternative inserts directives.
#define ALTINS_HEAD "~@"

QString KTranslit::resolveInserts (const QString &str_, int nins, int ind) const
{
    static QString head(ALTINS_HEAD);
    static int hlen = head.length();

    QString str = str_;
    QString rstr;
    while (1) {
        int p = str.indexOf(head);
        if (p < 0) {
            break;
        }

        // Append segment before optional insert to resulting text.
        rstr.append(str.left(p));

        // Must have at least 2 characters after the head.
        if (str.length() < p + hlen + 2) {
            kDebug(173) << QString("Malformed optional inserts list in {%1}, "
                                   "starting here: {%2}").arg(str_, str);
            return str_;
        }

        // Read the separating character and trim original string.
        QChar sep = str[p + hlen];
        str.remove(0, p + hlen + 1);

        // Parse requested number of inserts,
        // choose the one with matching index for resulting text.
        for (int i = 0; i < nins; ++i) {
            // Ending separator for this insert.
            int p = str.indexOf(sep);

            // Must have exactly the requested number of inserts.
            if (p < 0) {
                kDebug(173) << QString("Not enough inserts listed in {%1}, "
                                       "starting here: {%2}").arg(str_, str);
                return str_;
            }

            // If index is matching requested, append to resulting text.
            if (i == ind) {
                rstr.append(str.left(p));
            }

            // Trim original string.
            str.remove(0, p + 1);
        }
    }
    // Append the final segment to resulting text.
    rstr.append(str);

    return rstr;
}

// If the insert is just starting at position i, return the position of the
// first character after the insert (or string length if none).
// If the insert is not starting, return i itself.
static int skipInsert (const QString &str, int i, int ninserts)
{
    static QString head(ALTINS_HEAD);
    static int hlen = head.length();

    if (str.mid(i, hlen) == head) {
        int slen = str.length();
        int ia = i + hlen;
        if (ia >= slen) return slen;
        QChar sep = str[ia];
        for (int k = 0; k < ninserts; ++k) {
            ia = str.indexOf(sep, ia + 1);
            if (ia < 0) return slen;
        }
        return ia + 1;
    }
    else {
        return i;
    }
}

// -----------------------------------------------------------------------------
// Serbian.

class KTranslitSerbianPrivate
{
    public:
    QHash<QString, bool> latinNames;
    QHash<QChar, QString> dictC2L;
};

KTranslitSerbian::KTranslitSerbian ()
: d(new KTranslitSerbianPrivate())
{
    d->latinNames[QString::fromAscii("latin")] = true;
    d->latinNames[QString::fromAscii("Latn")] = true;

    #define SR_DICTC2L_ENTRY(a, b) do { \
        d->dictC2L[QString::fromUtf8(a)[0]] = QString::fromUtf8(b); \
    } while (0)
    SR_DICTC2L_ENTRY("а", "a");
    SR_DICTC2L_ENTRY("б", "b");
    SR_DICTC2L_ENTRY("в", "v");
    SR_DICTC2L_ENTRY("г", "g");
    SR_DICTC2L_ENTRY("д", "d");
    SR_DICTC2L_ENTRY("ђ", "đ");
    SR_DICTC2L_ENTRY("е", "e");
    SR_DICTC2L_ENTRY("ж", "ž");
    SR_DICTC2L_ENTRY("з", "z");
    SR_DICTC2L_ENTRY("и", "i");
    SR_DICTC2L_ENTRY("ј", "j");
    SR_DICTC2L_ENTRY("к", "k");
    SR_DICTC2L_ENTRY("л", "l");
    SR_DICTC2L_ENTRY("љ", "lj");
    SR_DICTC2L_ENTRY("м", "m");
    SR_DICTC2L_ENTRY("н", "n");
    SR_DICTC2L_ENTRY("њ", "nj");
    SR_DICTC2L_ENTRY("о", "o");
    SR_DICTC2L_ENTRY("п", "p");
    SR_DICTC2L_ENTRY("р", "r");
    SR_DICTC2L_ENTRY("с", "s");
    SR_DICTC2L_ENTRY("т", "t");
    SR_DICTC2L_ENTRY("ћ", "ć");
    SR_DICTC2L_ENTRY("у", "u");
    SR_DICTC2L_ENTRY("ф", "f");
    SR_DICTC2L_ENTRY("х", "h");
    SR_DICTC2L_ENTRY("ц", "c");
    SR_DICTC2L_ENTRY("ч", "č");
    SR_DICTC2L_ENTRY("џ", "dž");
    SR_DICTC2L_ENTRY("ш", "š");
    SR_DICTC2L_ENTRY("А", "A");
    SR_DICTC2L_ENTRY("Б", "B");
    SR_DICTC2L_ENTRY("В", "V");
    SR_DICTC2L_ENTRY("Г", "G");
    SR_DICTC2L_ENTRY("Д", "D");
    SR_DICTC2L_ENTRY("Ђ", "Đ");
    SR_DICTC2L_ENTRY("Е", "E");
    SR_DICTC2L_ENTRY("Ж", "Ž");
    SR_DICTC2L_ENTRY("З", "Z");
    SR_DICTC2L_ENTRY("И", "I");
    SR_DICTC2L_ENTRY("Ј", "J");
    SR_DICTC2L_ENTRY("К", "K");
    SR_DICTC2L_ENTRY("Л", "L");
    SR_DICTC2L_ENTRY("Љ", "Lj");
    SR_DICTC2L_ENTRY("М", "M");
    SR_DICTC2L_ENTRY("Н", "N");
    SR_DICTC2L_ENTRY("Њ", "Nj");
    SR_DICTC2L_ENTRY("О", "O");
    SR_DICTC2L_ENTRY("П", "P");
    SR_DICTC2L_ENTRY("Р", "R");
    SR_DICTC2L_ENTRY("С", "S");
    SR_DICTC2L_ENTRY("Т", "T");
    SR_DICTC2L_ENTRY("Ћ", "Ć");
    SR_DICTC2L_ENTRY("У", "U");
    SR_DICTC2L_ENTRY("Ф", "F");
    SR_DICTC2L_ENTRY("Х", "H");
    SR_DICTC2L_ENTRY("Ц", "C");
    SR_DICTC2L_ENTRY("Ч", "Č");
    SR_DICTC2L_ENTRY("Џ", "Dž");
    SR_DICTC2L_ENTRY("Ш", "Š");
}

KTranslitSerbian::~KTranslitSerbian ()
{
    delete d;
}

QString KTranslitSerbian::transliterate (const QString &str,
                                         const QString &script) const
{
    static QString insHead(ALTINS_HEAD);

    if (d->latinNames.contains(script)) {
        // NOTE: This loop has been somewhat optimized for speed.
        int slen = str.length();
        bool anyInserts = str.indexOf(insHead) >= 0;
        QString nstr;
        nstr.reserve(slen + 5);
        for (int i = 0; i < slen; ++i) {
            // Skip alternative inserts altogether, so that they can be used
            // as a mean to exclude from transliteration.
            if (anyInserts) {
                int to = skipInsert(str, i, 2);
                if (to > i) {
                    nstr.append(str.mid(i, to - i));
                    if (to >= slen) break;
                    i = to;
                }
            }
            // Transliterate current character.
            QChar c = str[i];
            QString r = d->dictC2L[c];
            if (!r.isEmpty()) {
                if (   r.length() > 1 && c.isUpper()
                    && (   (i + 1 < slen && str[i + 1].isUpper())
                        || (i > 0 && str[i - 1].isUpper()))) {
                    nstr.append(r.toUpper());
                }
                else {
                    nstr.append(r);
                }
            }
            else {
                nstr.append(c);
            }
        }
        nstr = resolveInserts(nstr, 2, 1);
        return nstr;
    }
    else {
        QString nstr = resolveInserts(str, 2, 0);
        return nstr;
    }
}
