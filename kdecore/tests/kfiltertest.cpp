/*
 *  Copyright (C) 2002-2005 David Faure   <faure@kde.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License version 2 as published by the Free Software Foundation;
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include "qtest_kde.h"

#include "kfilterdev.h"
#include "kfilterbase.h"
#include <unistd.h>
#include <limits.h>
#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <kdebug.h>
#include "kfiltertest.h"
#include <kgzipfilter.h>
#include <krandom.h>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <config.h>
#include <zlib.h>

QTEST_KDEMAIN_CORE(KFilterTest)

void KFilterTest::initTestCase()
{
    const QString currentdir = QDir::currentPath();
    pathgz = currentdir + "/test.gz";
    pathbz2 = currentdir + "/test.bz2";

    // warning, update the COMPAREs in test_block_write() if changing the test data...
    testData = "hello world\n";
}

void KFilterTest::test_block_write(const QString & fileName, const QByteArray& data)
{
    QIODevice * dev = KFilterDev::deviceForFile( fileName );
    QVERIFY( dev != 0 );
    bool ok = dev->open( QIODevice::WriteOnly );
    QVERIFY( ok );

    const int ret = dev->write(data);
    QCOMPARE(ret, data.size());

    dev->close();
    delete dev;

    QVERIFY( QFile::exists( fileName ) );
}

void KFilterTest::test_block_write()
{
    kDebug() << " -- test_block_write gzip -- ";
    test_block_write(pathgz, testData);
    QCOMPARE( QFileInfo( pathgz ).size(), 33LL ); // size of test.gz

    kDebug() << " -- test_block_write bzip2 -- ";
    test_block_write(pathbz2, testData);
    QCOMPARE( QFileInfo( pathbz2 ).size(), 52LL ); // size of test.bz2
}

void KFilterTest::test_biggerWrites()
{
    const QString currentdir = QDir::currentPath();
    const QString outFile = currentdir + "/test_big.gz";
    // Find the out-of-bounds from #157706/#188415
    QByteArray data;
    data.reserve(10000);
    // Prepare test data
    for (int i = 0; i < 8170; ++i)
        data.append((char)(KRandom::random() % 256));
    QCOMPARE(data.size(), 8170);
    // 8170 random bytes compress to 8194 bytes due to the gzip header/footer.
    // Now we can go one by one until we pass 8192.
    // On 32 bit systems it crashed with data.size()=8173, before the "no room for footer yet" fix.
    int compressedSize = 0;
    while (compressedSize < 8200) {
        test_block_write(outFile, data);
        compressedSize = QFileInfo(outFile).size();
        kDebug() << data.size() << "compressed into" << compressedSize;
        // Test data is valid
        test_readall(outFile, QString::fromLatin1("application/x-gzip"), data);


        data.append((char)(KRandom::random() % 256));
    }
}

void KFilterTest::test_block_read( const QString & fileName )
{
    QIODevice * dev = KFilterDev::deviceForFile( fileName );
    QVERIFY( dev != 0 );
    bool ok = dev->open( QIODevice::ReadOnly );
    QVERIFY( ok );

    QByteArray array(1024,'\0');
    QByteArray read;
    int n;
    while ( ( n = dev->read( array.data(), array.size() ) ) )
    {
        QVERIFY( n > 0 );
        read += QByteArray( array, n );
        //kDebug() << "read returned " << n;
        //kDebug() << "read='" << read << "'";

        // pos() has no real meaning on sequential devices
        // Ah, but kzip uses kfilterdev as a non-sequential device...

        QCOMPARE( (int)dev->pos(), (int)read.size() );
        //kDebug() << "dev.at = " << dev->at();
    }
    QCOMPARE( read, testData );

    // Test seeking back
    ok = dev->seek(0);
    // test readAll
    read = dev->readAll();
    QCOMPARE( read.size(), testData.size() );
    QCOMPARE( read, testData );

    dev->close();
    delete dev;
}

void KFilterTest::test_block_read()
{
    kDebug() << " -- test_block_read gzip -- ";
    test_block_read(pathgz);
    kDebug() << " -- test_block_read bzip2 -- ";
    test_block_read(pathbz2);
}

void KFilterTest::test_getch( const QString & fileName )
{
    QIODevice * dev = KFilterDev::deviceForFile( fileName );
    QVERIFY( dev != 0 );
    bool ok = dev->open( QIODevice::ReadOnly );
    QVERIFY( ok );
    QByteArray read;
    char ch;
    while ( dev->getChar(&ch) ) {
        //printf("%c",ch);
        read += ch;
    }
    dev->close();
    delete dev;
    QCOMPARE( read, testData );
}

void KFilterTest::test_getch()
{
    kDebug() << " -- test_getch gzip -- ";
    test_getch(pathgz);
    kDebug() << " -- test_getch bzip2 -- ";
    test_getch(pathbz2);
}

void KFilterTest::test_textstream(  const QString & fileName )
{
    QIODevice * dev = KFilterDev::deviceForFile( fileName );
    QVERIFY( dev != 0 );
    bool ok = dev->open( QIODevice::ReadOnly );
    QVERIFY( ok );
    QTextStream ts( dev );
    QString readStr = ts.readAll();
    dev->close();
    delete dev;

    QByteArray read = readStr.toLatin1();
    QCOMPARE( read, testData );
}

void KFilterTest::test_textstream()
{
    kDebug() << " -- test_textstream gzip -- ";
    test_textstream(pathgz);
    kDebug() << " -- test_textstream bzip2 -- ";
    test_textstream(pathbz2);
}

void KFilterTest::test_readall(const QString & fileName, const QString& mimeType, const QByteArray& expectedData)
{
    QFile file(fileName);
    QIODevice *flt = KFilterDev::device(&file, mimeType, false);
    bool ok = flt->open( QIODevice::ReadOnly );
    QVERIFY(ok);
    const QByteArray read = flt->readAll();
    QCOMPARE(read.size(), expectedData.size());
    QCOMPARE(read, expectedData);
    delete flt;
}

void KFilterTest::test_readall()
{
    kDebug() << " -- test_readall gzip -- ";
    test_readall(pathgz, QString::fromLatin1("application/x-gzip"), testData);
    kDebug() << " -- test_readall bzip2 -- ";
    test_readall(pathbz2, QString::fromLatin1("application/x-bzip"), testData);
    kDebug() << " -- test_readall gzip-derived -- ";
    test_readall(pathgz, QString::fromLatin1("image/svg+xml-compressed"), testData);
}

void KFilterTest::test_uncompressed()
{
    // Can KFilterDev handle uncompressed data even when using gzip decompression?
    kDebug() << " -- test_uncompressed -- ";
    QBuffer buffer(&testData);
    buffer.open(QIODevice::ReadOnly);
    QIODevice *flt = KFilterDev::device(&buffer, QString::fromLatin1("application/x-gzip"), false);
    bool ok = flt->open( QIODevice::ReadOnly );
    QVERIFY(ok);
    QByteArray read = flt->readAll();
    QCOMPARE( read.size(), testData.size() );
    QCOMPARE( read, testData );
    delete flt;
}

void KFilterTest::test_findFilterByMimeType_data()
{
    QTest::addColumn<QString>("mimeType");
    QTest::addColumn<bool>("valid");

    // direct mimetype name
    QTest::newRow("application/x-gzip") << QString::fromLatin1("application/x-gzip") << true;
#ifdef HAVE_BZIP2_SUPPORT
    QTest::newRow("application/x-bzip") << QString::fromLatin1("application/x-bzip") << true;
    QTest::newRow("application/x-bzip2") << QString::fromLatin1("application/x-bzip2") << true;
#else
    QTest::newRow("application/x-bzip") << QString::fromLatin1("application/x-bzip") << false;
    QTest::newRow("application/x-bzip2") << QString::fromLatin1("application/x-bzip2") << false;
#endif
    // indirect compressed mimetypes
    QTest::newRow("application/x-gzdvi") << QString::fromLatin1("application/x-gzdvi") << true;

    // non-compressed mimetypes
    QTest::newRow("text/plain") << QString::fromLatin1("text/plain") << false;
    QTest::newRow("application/x-tar") << QString::fromLatin1("application/x-tar") << false;
}

void KFilterTest::test_findFilterByMimeType()
{
    QFETCH(QString, mimeType);
    QFETCH(bool, valid);

    KFilterBase *filter = KFilterBase::findFilterByMimeType(mimeType);
    QCOMPARE(filter != 0, valid);

    delete filter;
}

void KFilterTest::test_deflateWithZlibHeader()
{
    QByteArray data = "Hello world, this is a test for deflate, from bug 114830 / 117683";
    QByteArray deflatedData;
    deflatedData.fill(0, long(data.size()*1.1f) + 12L); // requirements of zlib::compress2
    unsigned long out_bufferlen = deflatedData.size();
    int ret = compress2((Bytef*)deflatedData.data(), &out_bufferlen, (const Bytef*)data.constData(), data.size(), 1);
    QCOMPARE(ret, Z_OK);
    deflatedData.resize(out_bufferlen);

#if 0 // Can't use KFilterDev for this, we need to call KGzipFilter::init(QIODevice::ReadOnly, KGzipFilter::ZlibHeader);
    QBuffer buffer(&deflatedData);
    QIODevice *flt = KFilterDev::device(&buffer, "application/x-gzip", false);
    static_cast<KFilterDev *>(flt)->setSkipHeaders();
    bool ok = flt->open( QIODevice::ReadOnly );
    QVERIFY(ok);
    const QByteArray read = flt->readAll();
#else
    // Copied from HTTPFilter (which isn't linked into any kdelibs library)
    KGzipFilter* mFilterDevice = new KGzipFilter;
    mFilterDevice->init(QIODevice::ReadOnly, KGzipFilter::ZlibHeader);

    mFilterDevice->setInBuffer(deflatedData.constData(), deflatedData.size());
    char buf[8192];
    mFilterDevice->setOutBuffer(buf, sizeof(buf));
    KFilterBase::Result result = mFilterDevice->uncompress();
    QCOMPARE(result, KFilterBase::End);
    const int bytesOut = sizeof(buf) - mFilterDevice->outBufferAvailable();
    QVERIFY(bytesOut);
    QByteArray read(buf, bytesOut);
    mFilterDevice->terminate();
    delete mFilterDevice;
#endif
    QCOMPARE(QString::fromLatin1(read), QString::fromLatin1(data)); // more readable output than the line below
    QCOMPARE(read, data);
}

#include "kfiltertest.moc"
