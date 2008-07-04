/* This file is part of the KDE project
   Copyright (C) 2006 David Faure <faure@kde.org>

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

#include "kfileitemtest.h"
#include <qtest_kde.h>
#include "kfileitemtest.moc"
#include <kfileitem.h>

#include <ktempdir.h>
#include <ktemporaryfile.h>
#include <kuser.h>

QTEST_KDEMAIN( KFileItemTest, NoGUI )

void KFileItemTest::initTestCase()
{
}

void KFileItemTest::testPermissionsString()
{
    // Directory
    KTempDir tempDir;
    KFileItem dirItem(KUrl(tempDir.name()), QString(), KFileItem::Unknown);
    QCOMPARE((uint)dirItem.permissions(), (uint)0700);
    QCOMPARE(dirItem.permissionsString(), QString("drwx------"));
    QVERIFY(dirItem.isReadable());

    // File
    QFile file(tempDir.name() + "afile");
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ReadOther); // 0604
    KFileItem fileItem(KUrl(file.fileName()), QString(), KFileItem::Unknown);
    QCOMPARE((uint)fileItem.permissions(), (uint)0604);
    QCOMPARE(fileItem.permissionsString(), QString("-rw----r--"));
    QVERIFY(fileItem.isReadable());

    // Symlink
    QString symlink = tempDir.name() + "asymlink";
    QVERIFY( file.link( symlink ) );
    KUrl symlinkUrl(symlink);
    KFileItem symlinkItem(symlinkUrl, QString(), KFileItem::Unknown);
    QCOMPARE((uint)symlinkItem.permissions(), (uint)0604);
    // This is a bit different from "ls -l": we get the 'l' but we see the permissions of the target.
    // This is actually useful though; the user sees it's a link, and can check if he can read the [target] file.
    QCOMPARE(symlinkItem.permissionsString(), QString("lrw----r--"));
    QVERIFY(symlinkItem.isReadable());
}

void KFileItemTest::testNull()
{
    KFileItem null;
    QVERIFY(null.isNull());
    KFileItem fileItem(KUrl("/"), QString(), KFileItem::Unknown);
    QVERIFY(!fileItem.isNull());
    fileItem.mark();
    null = fileItem; // ok, now 'null' isn't so null anymore
    QVERIFY(!null.isNull());
    QVERIFY(null.isMarked());
    QVERIFY(null.isReadable());
}

void KFileItemTest::testDoesNotExist()
{
    KFileItem fileItem(KUrl("/doesnotexist"), QString(), KFileItem::Unknown);
    QVERIFY(!fileItem.isNull());
    QVERIFY(!fileItem.isReadable());
    QVERIFY(fileItem.user().isEmpty());
    QVERIFY(fileItem.group().isEmpty());
}

void KFileItemTest::testDetach()
{
    KFileItem fileItem(KUrl("/"), QString(), KFileItem::Unknown);
    fileItem.setExtraData(this, this);
    QCOMPARE(fileItem.extraData(this), (void*)this);
    KFileItem fileItem2(fileItem);
    QCOMPARE(fileItem2.extraData(this), (void*)this);
    QVERIFY(fileItem == fileItem2);
    fileItem2.mark();
    QVERIFY(fileItem2.isMarked());
    QVERIFY(!fileItem.isMarked());
    QVERIFY(fileItem != fileItem2);

    fileItem = fileItem2;
    QVERIFY(fileItem2.isMarked());
    QVERIFY(fileItem == fileItem2);
}

void KFileItemTest::testBasic()
{
    KTemporaryFile file;
    QVERIFY(file.open());
    QFile fileObj(file.fileName());
    QVERIFY(fileObj.open(QIODevice::WriteOnly));
    fileObj.write(QByteArray("Hello"));
    fileObj.close();

    KUrl url(file.fileName());
    KFileItem fileItem(url, QString(), KFileItem::Unknown);
    QCOMPARE(fileItem.text(), url.fileName());
    QVERIFY(fileItem.isLocalFile());
    QCOMPARE(fileItem.localPath(), url.path());
    QCOMPARE(fileItem.size(), KIO::filesize_t(5));
    QVERIFY(fileItem.linkDest().isEmpty());
    QVERIFY(!fileItem.isHidden());
    QVERIFY(fileItem.isReadable());
    QVERIFY(fileItem.isWritable());
    QVERIFY(fileItem.isFile());
    QVERIFY(!fileItem.isDir());
    QVERIFY(!fileItem.isDesktopFile());
    QCOMPARE(fileItem.user(), KUser().loginName());
#ifndef Q_OS_WIN
    QCOMPARE(fileItem.group(), KUserGroup().name());
#endif
}

void KFileItemTest::testMimeTypeOnDemand()
{
    KTemporaryFile file;
    QVERIFY(file.open());

    {
        KFileItem fileItem(KFileItem::Unknown, KFileItem::Unknown, KUrl(file.fileName()), true /*on demand*/);
        QCOMPARE(fileItem.mimeTypePtr()->name(), KMimeType::defaultMimeType());
        QVERIFY(!fileItem.isMimeTypeKnown());
        //kDebug() << fileItem.determineMimeType()->name();
        QCOMPARE(fileItem.determineMimeType()->name(), QString("application/x-zerosize"));
        QCOMPARE(fileItem.mimetype(), QString("application/x-zerosize"));
        QVERIFY(fileItem.isMimeTypeKnown());
    }

    {
        // Calling mimeType directly also does mimetype determination
        KFileItem fileItem(KFileItem::Unknown, KFileItem::Unknown, KUrl(file.fileName()), true /*on demand*/);
        QVERIFY(!fileItem.isMimeTypeKnown());
        QCOMPARE(fileItem.mimetype(), QString("application/x-zerosize"));
        QVERIFY(fileItem.isMimeTypeKnown());
    }

    {
        KTemporaryFile file;
        QVERIFY(file.open());
        // Check whether mime-magic is used.
        // No known extension, so it should be used by determineMimeType.
        file.write(QByteArray("%PDF-"));
        QString fileName = file.fileName();
        QVERIFY(!fileName.isEmpty());
        file.close();
        KFileItem fileItem(KFileItem::Unknown, KFileItem::Unknown, KUrl(fileName), true /*on demand*/);
        QCOMPARE(fileItem.mimeTypePtr()->name(), KMimeType::defaultMimeType());
        QVERIFY(!fileItem.isMimeTypeKnown());
        QCOMPARE(fileItem.determineMimeType()->name(), QString("application/pdf"));
        QCOMPARE(fileItem.mimetype(), QString("application/pdf"));
    }

    {
        KTemporaryFile file;
        file.setSuffix(".txt");
        QVERIFY(file.open());
        // Check whether mime-magic is used.
        // Known extension, so it should NOT be used.
        file.write(QByteArray("<smil"));
        QString fileName = file.fileName();
        QVERIFY(!fileName.isEmpty());
        file.close();
        KFileItem fileItem(KFileItem::Unknown, KFileItem::Unknown, KUrl(fileName), true /*on demand*/);
        QCOMPARE(fileItem.mimeTypePtr()->name(), QString("text/plain"));
        QVERIFY(fileItem.isMimeTypeKnown());
        QCOMPARE(fileItem.determineMimeType()->name(), QString("text/plain"));
        QCOMPARE(fileItem.mimetype(), QString("text/plain"));

        // And if the mimetype is not on demand?
        KFileItem fileItem2(KFileItem::Unknown, KFileItem::Unknown, KUrl(fileName));
        QCOMPARE(fileItem2.mimeTypePtr()->name(), QString("text/plain")); // XDG says: application/smil; but can't sniff all files so this can't work
        QVERIFY(fileItem2.isMimeTypeKnown());
    }
}

void KFileItemTest::testCmp()
{
    KTemporaryFile file;
    QVERIFY(file.open());

    KFileItem fileItem(KFileItem::Unknown, KFileItem::Unknown, KUrl(file.fileName()), true /*on demand*/);
    KFileItem fileItem2(KFileItem::Unknown, KFileItem::Unknown, KUrl(file.fileName()), false);
    QVERIFY(fileItem != fileItem2); // created independently so not 'equal'
    QVERIFY(fileItem.cmp(fileItem2));
}


void KFileItemTest::testDecodeFileName_data()
{
    QTest::addColumn<QString>("filename");
    QTest::addColumn<QString>("expectedText");

    QTest::newRow("simple") << "filename" << "filename";
    QTest::newRow("/ at end") << QString("foo") + QChar(0x2044) << QString("foo") + QChar(0x2044);
    QTest::newRow("/ at begin") << QString(QChar(0x2044)) << QString(QChar(0x2044));
}


void KFileItemTest::testDecodeFileName()
{
    QFETCH(QString, filename);
    QFETCH(QString, expectedText);
    QCOMPARE(KIO::decodeFileName(filename), expectedText);
}

void KFileItemTest::testEncodeFileName_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("expectedFileName");

    QTest::newRow("simple") << "filename" << "filename";
    QTest::newRow("/ at end") << "foo/" << QString("foo") + QChar(0x2044);
    QTest::newRow("/ at begin") << "/" << QString(QChar(0x2044));
}

void KFileItemTest::testEncodeFileName()
{
    QFETCH(QString, text);
    QFETCH(QString, expectedFileName);
    QCOMPARE(KIO::encodeFileName(text), expectedFileName);
}

// TODO test KFileItem(UDSEntry), for instance doing a synchronous kio listing or stat.
