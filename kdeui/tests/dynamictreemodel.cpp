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

#include "dynamictreemodel.h"

#include <QHash>
#include <QList>
#include <QTimer>

#include <QDebug>

#include <kdebug.h>

DynamicTreeModel::DynamicTreeModel(QObject *parent)
  : QAbstractItemModel(parent),
    nextId(1)
{
}

QModelIndex DynamicTreeModel::index(int row, int column, const QModelIndex &parent) const
{
//   if (column != 0)
//     return QModelIndex();


  if ( column < 0 || row < 0 )
    return QModelIndex();

  QList<QList<qint64> > childIdColumns = m_childItems.value(parent.internalId());


  if (childIdColumns.size() == 0)
    return QModelIndex();

  if (column >= childIdColumns.size())
    return QModelIndex();

  QList<qint64> rowIds = childIdColumns.at(column);

  if ( row >= rowIds.size())
    return QModelIndex();

  qint64 id = rowIds.at(row);

  return createIndex(row, column, reinterpret_cast<void *>(id));

}

qint64 DynamicTreeModel::findParentId(qint64 searchId) const
{
  if (searchId <= 0)
    return -1;

  QHashIterator<qint64, QList<QList<qint64> > > i(m_childItems);
  while (i.hasNext())
  {
    i.next();
    QListIterator<QList<qint64> > j(i.value());
    while (j.hasNext())
    {
      QList<qint64> l = j.next();
      if (l.contains(searchId))
      {
        return i.key();
      }
    }
  }
  return -1;
}

QModelIndex DynamicTreeModel::parent(const QModelIndex &index) const
{
  if (!index.isValid())
    return QModelIndex();

  qint64 searchId = index.internalId();
  qint64 parentId = findParentId(searchId);
  // Will never happen for valid index, but what the hey...
  if (parentId <= 0)
    return QModelIndex();

  qint64 grandParentId = findParentId(parentId);
  if (grandParentId < 0)
    grandParentId = 0;

  int column = 0;
  QList<qint64> childList = m_childItems.value(grandParentId).at(column);

  int row = childList.indexOf(parentId);

  return createIndex(row, column, reinterpret_cast<void *>(parentId));

}

int DynamicTreeModel::rowCount(const QModelIndex &index ) const
{
  QList<QList<qint64> > cols = m_childItems.value(index.internalId());

  if (cols.size() == 0 )
    return 0;

  if (index.column() > 0)
    return 0;

  return cols.at(0).size();
}

int DynamicTreeModel::columnCount(const QModelIndex &index ) const
{
//   Q_UNUSED(index);
  return m_childItems.value(index.internalId()).size();
}

QVariant DynamicTreeModel::data(const QModelIndex &index, int role) const
{
  if (!index.isValid())
    return QVariant();

  if (Qt::DisplayRole == role)
  {
    return m_items.value(index.internalId());
  }
  return QVariant();
}

void DynamicTreeModel::clear()
{
  m_items.clear();
  m_childItems.clear();
  nextId = 1;
  reset();
}


ModelChangeCommand::ModelChangeCommand( DynamicTreeModel *model, QObject *parent )
    : QObject(parent), m_model(model), m_numCols(1), m_startRow(-1), m_endRow(-1)
{

}

QModelIndex ModelChangeCommand::findIndex(QList<int> rows)
{
  const int col = 0;
  QModelIndex parent = QModelIndex();
  QListIterator<int> i(rows);
  while (i.hasNext())
  {
    parent = m_model->index(i.next(), col, parent);
    Q_ASSERT(parent.isValid());
  }
  return parent;
}

ModelInsertCommand::ModelInsertCommand(DynamicTreeModel *model, QObject *parent )
    : ModelChangeCommand(model, parent)
{

}

void ModelInsertCommand::doCommand()
{
  QModelIndex parent = findIndex(m_rowNumbers);
  m_model->beginInsertRows(parent, m_startRow, m_endRow);
  qint64 parentId = parent.internalId();
  for (int row = m_startRow; row <= m_endRow; row++)
  {
    for(int col = 0; col < m_numCols; col++ )
    {
      if (m_model->m_childItems[parentId].size() <= col)
      {
        m_model->m_childItems[parentId].append(QList<qint64>());
      }
//       QString name = QUuid::createUuid().toString();
      qint64 id = m_model->newId();
      QString name = QString::number(id);

      m_model->m_items.insert(id, name);
      m_model->m_childItems[parentId][col].insert(row, id);

    }
  }
  m_model->endInsertRows();
}

ModelInsertWithDescendantsCommand::ModelInsertWithDescendantsCommand(DynamicTreeModel *model, QObject *parent)
    : ModelInsertCommand(model, parent)
{

}

void ModelInsertWithDescendantsCommand::setFragments(QList<InsertFragment> fragments)
{
  m_fragments = fragments;
}

void ModelInsertWithDescendantsCommand::insertFragment(qint64 parentIdentifier, InsertFragment fragment)
{
  for(int col = 0; col < m_numCols; col++ )
  {
    if (m_model->m_childItems[parentIdentifier].size() <= col)
    {
      m_model->m_childItems[parentIdentifier].append(QList<qint64>());
    }
    for (int row = 0; row < fragment.numRows; row++)
    {
      qint64 id = m_model->newId();
      QString name = QString::number(id);

      m_model->m_items.insert(id, name);
      m_model->m_childItems[parentIdentifier][col].append(id);

      if (col == 0 && fragment.subfragments.contains(row) )
      {
        insertFragment(id, fragment.subfragments.take(row));
      }
    }
  }

}

void ModelInsertWithDescendantsCommand::doCommand()
{
  QModelIndex fragmentParent = findIndex(m_rowNumbers);

  qint64 fragmentParentIdentifier = fragmentParent.internalId();

  const int column = 0;

  qint64 rootIdentifier = m_model->m_childItems[fragmentParentIdentifier][column][m_startRow];
  QModelIndex fragmentIndex = m_model->index(m_startRow, column, fragmentParent);

  InsertFragment fragment = m_fragments.at(0);

  m_model->beginInsertRows(fragmentIndex, 0, fragment.numRows - 1);
  insertFragment(rootIdentifier, fragment);
  m_model->endInsertRows();
}


ModelRemoveCommand::ModelRemoveCommand(DynamicTreeModel *model, QObject *parent )
    : ModelChangeCommand(model, parent)
{

}

void ModelRemoveCommand::doCommand()
{
  QModelIndex parent = findIndex(m_rowNumbers);
  m_model->beginRemoveRows(parent, m_startRow, m_endRow);
  qint64 parentId = parent.internalId();
  for(int col = 0; col < m_numCols; col++ )
  {
    QList<qint64> childItems = m_model->m_childItems.value(parentId).value(col);
    for (int row = m_startRow; row <= m_endRow; row++)
    {
      qint64 item = childItems[row];
      purgeItem(item);
      m_model->m_childItems[parentId][col].removeOne(item);
    }
  }
  m_model->endRemoveRows();
}

void ModelRemoveCommand::purgeItem(qint64 parent)
{
  QList<QList<qint64> > childItemRows = m_model->m_childItems.value(parent);

  if (childItemRows.size() > 0)
  {
    for (int col = 0; col < m_numCols; col++)
    {
      QList<qint64> childItems = childItemRows[col];
      foreach(qint64 item, childItems)
      {
        purgeItem(item);
        m_model->m_childItems[parent][col].removeOne(item);
      }
    }
  }
  m_model->m_items.remove(parent);
}


ModelDataChangeCommand::ModelDataChangeCommand(DynamicTreeModel *model, QObject *parent)
  : ModelChangeCommand(model, parent), m_startColumn(0)
{

}

void ModelDataChangeCommand::doCommand()
{
  QModelIndex parent = findIndex(m_rowNumbers);
  QModelIndex topLeft = m_model->index(m_startRow, m_startColumn, parent);
  QModelIndex bottomRight = m_model->index(m_endRow, m_numCols - 1, parent);

  QList<QList<qint64> > childItems = m_model->m_childItems[parent.internalId()];


  for (int col = m_startColumn; col < m_startColumn + m_numCols; col++)
  {
    for (int row = m_startRow; row <= m_endRow; row++ )
    {
      QString name = QString::number( m_model->newId() );
      m_model->m_items[childItems[col][row]] = name;
    }
  }
  m_model->dataChanged(topLeft, bottomRight);
}


ModelMoveCommand::ModelMoveCommand(DynamicTreeModel *model, QObject *parent)
  : ModelChangeCommand(model, parent)
{

}

void ModelMoveCommand::doCommand()
{
  QModelIndex srcParent = findIndex(m_rowNumbers);
  QModelIndex destParent = findIndex(m_destRowNumbers);

  // TODO: Uncomment for Qt 4.6
//   if (!m_model->beginMoveRows(srcParent, m_startRow, m_endRow, destParent, m_destRow))
//   {
//     return;
//   }
  
  for (int column = 0; column < m_numCols; ++column)
  {
    QList<qint64> l = m_model->m_childItems.value(srcParent.internalId())[column].mid(m_startRow, m_endRow - m_startRow + 1 );

    for (int i = m_startRow; i <= m_endRow ; i++)
    {
      m_model->m_childItems[srcParent.internalId()][column].removeAt(m_startRow);
    }
    int d;
    if (m_destRow < m_startRow)
      d = m_destRow;
    else
    {
      if (srcParent == destParent)
        d = m_destRow - (m_endRow - m_startRow + 1);
      else
        d = m_destRow - (m_endRow - m_startRow) + 1;
    }

    foreach(const qint64 id, l)
    {
      if (m_model->m_childItems[destParent.internalId()].size() <= column)
      {
        m_model->m_childItems[destParent.internalId()].append(QList<qint64>());
      }
      m_model->m_childItems[destParent.internalId()][column].insert(d++, id);
    }
  }

  // TODO: Uncomment for Qt 4.6
//   m_model->endMoveRows();
}
