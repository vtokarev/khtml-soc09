#include "globals.h"

#include "speller.h"
#include "filter_p.h"

#include <QMap>
#include <QVector>
#include <QSet>
#include <QDebug>

namespace Sonnet {
/* a very silly implementation: uses all available dictionaries
 * to check the text, the one with the least amount of misspelling
 * is likely the language we're looking for. (unless of course
 * someone is a real terrible speller).
 */
QString detectLanguage(const QString &sentence)
{
    Speller speller;
    QMap<QString, QString> dicts = speller.availableDictionaries();
    QMap<QString, int> correctHits;
    QSet<QString> seenLangs;
    {
        QMap<QString, QString>::const_iterator itr = dicts.constBegin();
        for (int i = 0; i < dicts.count(); ++i) {
            seenLangs.insert(itr.value());
            ++itr;
        }
    }

    QVector<Speller> spellers(seenLangs.count());
    {
        QSet<QString>::const_iterator itr = seenLangs.constBegin();
        for (int i = 0; i < spellers.count(); ++i) {
            spellers[i].setLanguage(*itr);
            ++itr;
        }
    }

    Filter f;
    f.setBuffer(sentence);
    while (!f.atEnd()) {
        Word word = f.nextWord();

        if (!word.word.isEmpty()) {
            for (int i = 0; i < spellers.count(); ++i) {
                if (spellers[i].isCorrect(word.word))
                    correctHits[spellers[i].language()] += 1;
            }
        }
    }

    QMap<QString, int>::const_iterator min = correctHits.constBegin();
    for (QMap<QString, int>::const_iterator itr = correctHits.constBegin();
         itr != correctHits.constEnd(); ++itr) {
        if (itr.value() < min.value())
            min = itr;
    }
    return min.key();
}

}
