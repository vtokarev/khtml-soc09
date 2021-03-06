#ifndef _MEIN_XSLT_H_
#define _MEIN_XSLT_H_

#include <libxml/parser.h>
#include <QtCore/QString>
#include <QtCore/QVector>

class QByteArray;
class KComponentData;

QString transform(const QString &file, const QString& stylesheet,
                  const QVector<const char *> &params = QVector<const char *>());
QString splitOut(const QString &parsed, int index);
void fillInstance(KComponentData &ins, const QString &srcdir = QString() );
bool saveToCache( const QString &contents, const QString &filename );
QString lookForCache( const QString &filename );
QByteArray fromUnicode( const QString &data );
void replaceCharsetHeader( QString &output );

extern bool warnings_exist;
extern QString *SRCDIR;

/**
 * Compares two files and returns true if @param newer exists and is newer than
 * @param older
 **/
bool compareTimeStamps( const QString &older, const QString &newer );
#endif
