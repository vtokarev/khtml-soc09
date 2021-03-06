/*
 *
 * $Id: sourceheader 511311 2006-02-19 14:51:05Z trueg $
 *
 * This file is part of the Nepomuk KDE project.
 * Copyright (C) 2006-2007 Sebastian Trueg <trueg@kde.org>
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING.LIB" for the exact licensing terms.
 */

#include "resourcetest.h"

#include "../resource.h"
#include "../variant.h"
#include "../resourcemanager.h"

#include <kdebug.h>
#include <qtest_kde.h>

#include <Soprano/Soprano>

#include <QtCore/QTextStream>

using namespace Soprano;
using namespace Nepomuk;


void ResourceTest::testResourceStates()
{
    QUrl someUri = ResourceManager::instance()->generateUniqueUri( QString() );

    Resource r1( someUri );
    Resource r2( someUri );

    QCOMPARE( r1, r2 );

    QVERIFY( r1.isValid() );
    QVERIFY( !r1.exists() );

    r1.setProperty( QUrl("http://test/something"), 12 );

    QCOMPARE( r1, r2 );
    QVERIFY( r1.exists() );
}


void ResourceTest::testResourceRemoval()
{
    Resource res( "testi" );
    res.setProperty( QUrl("http://nepomuk.test.org/foo/bar"),  "foobar" );

    QVERIFY( !res.resourceUri().isEmpty() );

    QVERIFY( ResourceManager::instance()->mainModel()->containsAnyStatement( Statement( res.resourceUri(), Node(), Node() ) ) );

    res.remove();

    QVERIFY( !res.exists() );

    QVERIFY( !ResourceManager::instance()->mainModel()->containsAnyStatement( Statement( res.resourceUri(), Node(), Node() ) ) );

    //
    // test recursive removal
    //
    Resource res2( "testi2" );

    res.setProperty( QUrl("http://nepomuk.test.org/foo/bar2"), res2 );

    QVERIFY( res.exists() );
    QVERIFY( res2.exists() );

    QVERIFY( ResourceManager::instance()->mainModel()->containsAnyStatement( Statement( res.resourceUri(), QUrl("http://nepomuk.test.org/foo/bar2"), Node(res2.resourceUri()) ) ) );

    res2.remove();

    QVERIFY( res.exists() );
    QVERIFY( !res2.exists() );

    QVERIFY( !ResourceManager::instance()->mainModel()->containsAnyStatement( Statement( res.resourceUri(), QUrl("http://nepomuk.test.org/foo/bar2"), Node(res2.resourceUri()) ) ) );
}


void ResourceTest::testProperties()
{
    QUrl r1Uri, r2Uri;

    {
        Resource r1( "testi" );
        Resource r2( "testi2" );

        r1.setProperty( QUrl("http://nepomuk.test.org/int"), 17 );
        r1.setProperty( QUrl("http://nepomuk.test.org/bool1"), true );
        r1.setProperty( QUrl("http://nepomuk.test.org/bool2"), false );
        r1.setProperty( QUrl("http://nepomuk.test.org/double"), 2.2 );
        r1.setProperty( QUrl("http://nepomuk.test.org/string"), "test" );
        r1.setProperty( QUrl("http://nepomuk.test.org/date"), QDate::currentDate() );
        r1.setProperty( QUrl("http://nepomuk.test.org/Resource"), r2 );

        r1Uri = r1.resourceUri();
        r2Uri = r2.resourceUri();
    }

    QTextStream s(stdout);
    foreach( const Statement& st,     ResourceManager::instance()->mainModel()->listStatements().allStatements() ) {
        s << st << endl;
    }

    {
        Resource r1( r1Uri );
        Resource r2( r2Uri );

        QVERIFY( r1.hasProperty( QUrl("http://nepomuk.test.org/int" ) ) );
        QVERIFY( r1.hasProperty( QUrl("http://nepomuk.test.org/bool1" ) ) );
        QVERIFY( r1.hasProperty( QUrl("http://nepomuk.test.org/bool2" ) ) );
        QVERIFY( r1.hasProperty( QUrl("http://nepomuk.test.org/double" ) ) );
        QVERIFY( r1.hasProperty( QUrl("http://nepomuk.test.org/string" ) ) );
        QVERIFY( r1.hasProperty( QUrl("http://nepomuk.test.org/date" ) ) );
        QVERIFY( r1.hasProperty( QUrl("http://nepomuk.test.org/Resource" ) ) );

        QCOMPARE( r1.property( QUrl("http://nepomuk.test.org/int" ) ).toInt(), 17 );
        QCOMPARE( r1.property( QUrl("http://nepomuk.test.org/bool1" ) ).toBool(), true );
        QCOMPARE( r1.property( QUrl("http://nepomuk.test.org/bool2" ) ).toBool(), false );
        QCOMPARE( r1.property( QUrl("http://nepomuk.test.org/double" ) ).toDouble(), 2.2 );
        QCOMPARE( r1.property( QUrl("http://nepomuk.test.org/string" ) ).toString(), QString("test") );
        QCOMPARE( r1.property( QUrl("http://nepomuk.test.org/date" ) ).toDate(), QDate::currentDate() );
        QCOMPARE( r1.property( QUrl("http://nepomuk.test.org/Resource" ) ).toResource(), r2 );

        QHash<QString, Variant> allProps = r1.allProperties();
        QCOMPARE( allProps.count(), 10 ); // properties + type + identifier + modification date
        QVERIFY( allProps.keys().contains( "http://nepomuk.test.org/int" ) );
        QVERIFY( allProps.keys().contains( "http://nepomuk.test.org/bool1" ) );
        QVERIFY( allProps.keys().contains( "http://nepomuk.test.org/bool2" ) );
        QVERIFY( allProps.keys().contains( "http://nepomuk.test.org/double" ) );
        QVERIFY( allProps.keys().contains( "http://nepomuk.test.org/string" ) );
        QVERIFY( allProps.keys().contains( "http://nepomuk.test.org/date" ) );
        QVERIFY( allProps.keys().contains( "http://nepomuk.test.org/Resource" ) );
    }
}


void ResourceTest::testResourceIdentifiers()
{
    QUrl theUri;
    {
        Resource r1( "wurst" );
        Resource r2( "wurst" );

        QVERIFY( r1 == r2 );

        theUri = r1.resourceUri();

        Resource r3( r1.resourceUri() );

        QVERIFY( r1 == r3 );

        QVERIFY( r1.resourceUri() != QUrl("wurst") );

        r1.setProperty( QUrl("http://nepomuk.test.org/foo/bar"), "foobar" );

        QList<Statement> sl
            = ResourceManager::instance()->mainModel()->listStatements( Statement( r1.resourceUri(), Node(), Node() ) ).allStatements();

        foreach( const Statement& s, sl )
            qDebug() << s << endl;

        QCOMPARE( sl.count(), 4 );

        QVERIFY( ResourceManager::instance()->mainModel()->containsAnyStatement( Statement( r1.resourceUri(),
                                                                                            QUrl( Resource::identifierUri() ),
                                                                                            LiteralValue( "wurst" ) ) ) );
    }

    {
        Resource r1( theUri );
        Resource r2( "wurst" );

        QCOMPARE( r1, r2 );
    }
}


void ResourceTest::testResourceManager()
{
    {
        Resource r1( "res1", QUrl("http://test/mytype" ) );
        Resource r2( "res2", QUrl("http://test/mytype" ) );
        Resource r3( "res3", QUrl("http://test/myothertype" ) );
        Resource r4( "res4", QUrl("http://test/myothertype" ) );
        Resource r5( "res5", QUrl("http://test/myothertype" ) );
        Resource r6( "res6", QUrl("http://test/mythirdtype" ) );

        QList<Resource> rl = ResourceManager::instance()->allResourcesOfType( QUrl("http://test/mytype") );
        QCOMPARE( rl.count(), 2 );
        QVERIFY( rl.contains( r1 ) && rl.contains( r2 ) );

        rl = ResourceManager::instance()->allResourcesOfType( r6.resourceType() );
        QCOMPARE( rl.count(), 1 );
        QCOMPARE( rl.first(), r6 );

        r1.setProperty( QUrl("http://test/prop1"), 42 );
        r3.setProperty( QUrl("http://test/prop1"), 42 );
        r4.setProperty( QUrl("http://test/prop1"), 41 );

        r3.setProperty( QUrl("http://test/prop2"), r6 );
        r4.setProperty( QUrl("http://test/prop2"), r6 );
        r5.setProperty( QUrl("http://test/prop2"), r6 );
        r6.setProperty( QUrl("http://test/prop2"), r1 );

        rl = ResourceManager::instance()->allResourcesWithProperty( QUrl("http://test/prop1"), 42 );
        QCOMPARE( rl.count(), 2 );
        QVERIFY( rl.contains( r1 ) && rl.contains( r3 ) );

        rl = ResourceManager::instance()->allResourcesWithProperty( QUrl("http://test/prop2"), r6 );
        QCOMPARE( rl.count(), 3 );
        QVERIFY( rl.contains( r3 ) && rl.contains( r4 ) && rl.contains( r5 ) );
    }

    {
        Resource r1( "res1", QUrl("http://test/mytype" ) );
        Resource r2( "res2", QUrl("http://test/mytype" ) );
        Resource r3( "res3", QUrl("http://test/myothertype" ) );
        Resource r4( "res4", QUrl("http://test/myothertype" ) );
        Resource r5( "res5", QUrl("http://test/myothertype" ) );
        Resource r6( "res6", QUrl("http://test/mythirdtype" ) );

        QList<Resource> rl = ResourceManager::instance()->allResourcesOfType( QUrl("http://test/mytype" ));
        QCOMPARE( rl.count(), 2 );
        QVERIFY( rl.contains( r1 ) && rl.contains( r2 ) );

        rl = ResourceManager::instance()->allResourcesOfType( r6.resourceType() );
        QCOMPARE( rl.count(), 1 );
        QCOMPARE( rl.first(), r6 );

        rl = ResourceManager::instance()->allResourcesWithProperty( QUrl("http://test/prop1"), 42 );
        QCOMPARE( rl.count(), 2 );
        QVERIFY( rl.contains( r1 ) && rl.contains( r3 ) );

        rl = ResourceManager::instance()->allResourcesWithProperty( QUrl("http://test/prop2"), r6 );
        QCOMPARE( rl.count(), 3 );
        QVERIFY( rl.contains( r3 ) && rl.contains( r4 ) && rl.contains( r5 ) );

        QVERIFY( r1.hasProperty( QUrl("http://test/prop1" ) ) );
        QVERIFY( r3.hasProperty( QUrl("http://test/prop1" ) ) );
        QVERIFY( r4.hasProperty( QUrl("http://test/prop1" ) ) );

        QVERIFY( r3.hasProperty( QUrl("http://test/prop2" ) ) );
        QVERIFY( r4.hasProperty( QUrl("http://test/prop2" ) ) );
        QVERIFY( r5.hasProperty( QUrl("http://test/prop2" ) ) );
        QVERIFY( r6.hasProperty( QUrl("http://test/prop2" ) ) );

        QCOMPARE( r3.property( QUrl("http://test/prop2" )).toResource(), r6 );
        QCOMPARE( r4.property( QUrl("http://test/prop2" )).toResource(), r6 );
        QCOMPARE( r5.property( QUrl("http://test/prop2" )).toResource(), r6 );
        QCOMPARE( r6.property( QUrl("http://test/prop2" )).toResource(), r1 );
    }
}


QTEST_KDEMAIN(ResourceTest, NoGUI)

#include "resourcetest.moc"
