/*
    Copyright (c) 2009 Stephen Kelly <steveire@gmail.com>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include <QtTest>
#include <QtCore>
#include <qtest_kde.h>
#include <qtestevent.h>

#include "dynamictreemodel.h"
#include "kdescendantsproxymodel.h"
#include "proxymodeltest.h"

#include <QTreeView>

#include <kdebug.h>

class DescendantsProxyModelTest : public ProxyModelTest
{
  Q_OBJECT
public:
  DescendantsProxyModelTest( QObject *parent = 0 )
      : ProxyModelTest( parent )
  {
  }

  protected:
    QVariantList getSignal(SignalType type, int start, int end)
    {
      return ProxyModelTest::getSignal(type, IndexFinder(), start, end);
    }

    void signalInsertion(const QString &name, int startRow, int rowsAffected)
    {
      ProxyModelTest::signalInsertion(name, IndexFinder(), startRow, rowsAffected, m_rowCount);
      m_rowCount += rowsAffected;
    }

    void signalMove(const QString &name, int startRow, int endRow, int destRow)
    {
      ProxyModelTest::signalMove(name, IndexFinder(), startRow, endRow, IndexFinder(), destRow);
    }

    void signalRemoval(const QString &name, int startRow, int rowsAffected)
    {
      ProxyModelTest::signalRemoval(name, IndexFinder(), startRow, rowsAffected, m_rowCount);
      m_rowCount -= rowsAffected;
    }

    void signalDataChange(const QString &name, int startRow, int endRow)
    {
      IndexFinder topLeftFinder(m_proxyModel, QList<int>() << startRow );
      IndexFinder bottomRightFinder(m_proxyModel, QList<int>() << endRow );
      ProxyModelTest::signalDataChange(name, topLeftFinder, bottomRightFinder);
    }

    QVariantList getDataChangedSignal(int startRow, int endRow)
    {
      IndexFinder topLeftFinder(m_proxyModel, QList<int>() << startRow );
      IndexFinder bottomRightFinder(m_proxyModel, QList<int>() << endRow );

      return QVariantList() << DataChanged << QVariant::fromValue(topLeftFinder) << QVariant::fromValue(bottomRightFinder);
    }

    PersistentIndexChange getChange(int start, int end, int difference, bool toInvalid = false)
    {
      return ProxyModelTest::getChange(IndexFinder(), start, end, difference, toInvalid);
    }

private slots:
  void initTestCase();

private:
  KDescendantsProxyModel *m_proxyModel;
  IndexFinder m_rootIdxFinder;
  int m_rowCount;
};


void DescendantsProxyModelTest::initTestCase()
{
  m_proxyModel = new KDescendantsProxyModel(this);
  setProxyModel(m_proxyModel);

  QList<QVariantList> signalList;
  QVariantList expected;
  QList<PersistentIndexChange> persistentList;
  m_rowCount = 0;
  int startRow = 0;
  int rowsInserted = 1;

  signalInsertion("insert01", startRow, rowsInserted);

  startRow = 1;
  rowsInserted = 10;
  signalInsertion("insert02", startRow, rowsInserted);

  startRow = 0;
  signalInsertion("insert03", startRow, rowsInserted);

  startRow = 21;
  signalInsertion("insert04", startRow, rowsInserted);

  startRow = 11;
  signalInsertion("insert05", startRow, rowsInserted);

  startRow = 31;
  signalInsertion("insert06", startRow, rowsInserted);

  startRow = 21;
  signalInsertion("insert07", startRow, rowsInserted);

  int accumulatedChange = 0;
  startRow = 17;
  signalList << getSignal(RowsAboutToBeInserted, startRow, startRow + rowsInserted - 1);
  signalList << getSignal(RowsInserted, startRow, startRow + rowsInserted - 1);
  accumulatedChange += 10;

  startRow = 23;
  signalList << getSignal(RowsAboutToBeInserted, startRow, startRow + rowsInserted - 1);
  signalList << getSignal(RowsInserted, startRow, startRow + rowsInserted - 1);
  accumulatedChange += 10;

  startRow = 29;
  signalList << getSignal(RowsAboutToBeInserted, startRow, startRow + rowsInserted - 1);
  signalList << getSignal(RowsInserted, startRow, startRow + rowsInserted - 1);
  accumulatedChange += 10;
  persistentList << getChange(17, 17, accumulatedChange);

  startRow = 48;
  signalList << getSignal(RowsAboutToBeInserted, startRow, startRow + rowsInserted - 1);
  signalList << getSignal(RowsInserted, startRow, startRow + rowsInserted - 1);
  accumulatedChange += 10;
  persistentList << getChange(18, 18, accumulatedChange);

  startRow = 59;
  signalList << getSignal(RowsAboutToBeInserted, startRow, startRow + rowsInserted - 1);
  signalList << getSignal(RowsInserted, startRow, startRow + rowsInserted - 1);
  accumulatedChange += 10;

  persistentList << getChange(19, m_rowCount - 1, accumulatedChange);

  setExpected("insert08", signalList, persistentList);
  signalList.clear();
  persistentList.clear();
  m_rowCount += accumulatedChange;

  // When this proxy recieves a rowsAboutToBeInserted signal, it can't know
  // how many rows need to be inserted (total descendants).
  // So, it first inserts only the rows signaled by the source model (and not
  // the descendants). When the source model signals rowsInserted, we can
  // examine the new rows for descendants. These of need to be signalled separately
  // by this proxy

  startRow = 14;
  signalList << getSignal(RowsAboutToBeInserted, startRow, startRow + rowsInserted -1);
  signalList << getSignal(RowsInserted, startRow, startRow + rowsInserted -1);

  startRow = 17;
  rowsInserted = 20;
  signalList << getSignal(RowsAboutToBeInserted, startRow, startRow + rowsInserted -1);
  signalList << getSignal(RowsInserted, startRow, startRow + rowsInserted -1);

  startRow = 40;
  rowsInserted = 20;
  signalList << getSignal(RowsAboutToBeInserted, startRow, startRow + rowsInserted -1);
  signalList << getSignal(RowsInserted, startRow, startRow + rowsInserted -1);

  persistentList << getChange(14, m_rowCount - 1, 50);

  setExpected("insert09", signalList, persistentList);
  signalList.clear();
  persistentList.clear();
  m_rowCount += 50;

  startRow = 31;
  int endRow = 31;
  int destRow = 36;
  signalMove("move01", startRow, endRow, destRow);


  startRow = 36;
  endRow = 36;
  destRow = 31;
  signalMove("move02", startRow, endRow, destRow);

  startRow = 11;
  signalDataChange("change01", startRow, startRow);

  // Although the source model emits only one range is changed, this proxy model puts children indexes
  // in the way, breaking the continuous range.
  // Currently separate signals are emitted for each changed row.
  // This should really emit one signal for each continuous range instead. That's a TODO.
  startRow = 65;
  endRow = 65;
  signalList << getDataChangedSignal(startRow, endRow);

  startRow = 66;
  endRow = 66;
  signalList << getDataChangedSignal(startRow, endRow);

  startRow = 97;
  endRow = 97;
  signalList << getDataChangedSignal(startRow, endRow);

  startRow = 108;
  endRow = 108;
  signalList << getDataChangedSignal(startRow, endRow);

  setExpected("change02", signalList, persistentList);
  signalList.clear();

  startRow = 11;
  int rowsRemoved = 1;
  signalRemoval("remove01", startRow, rowsRemoved);

  startRow = 107;
  rowsRemoved = 11;
  signalRemoval("remove02", startRow, rowsRemoved);

  startRow = 97;
  rowsRemoved = 1;
  signalRemoval("remove03", startRow, rowsRemoved);

  startRow = 105;
  signalRemoval("remove04", startRow, rowsRemoved);

  startRow = 100;
  signalRemoval("remove05", startRow, rowsRemoved);

  startRow = 97;
  rowsRemoved = 7;
  signalRemoval("remove06", startRow, rowsRemoved);

  startRow = 65;
  rowsRemoved = 31;
  signalRemoval("remove07", startRow, rowsRemoved);

}

QTEST_KDEMAIN(DescendantsProxyModelTest, GUI)
#include "kdescendantentitiesproxymodeltest.moc"
