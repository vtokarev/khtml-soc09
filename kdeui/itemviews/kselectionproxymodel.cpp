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

#include "kselectionproxymodel.h"

#include <QItemSelectionRange>

class KSelectionProxyModelPrivate
{
public:
  KSelectionProxyModelPrivate(KSelectionProxyModel *model)
    : q_ptr(model),
      m_startWithChildTrees(false),
      m_omitChildren(false),
      m_omitDescendants(false),
      m_includeAllSelected(false),
      m_rowsRemoved(false)
  {

  }

  Q_DECLARE_PUBLIC(KSelectionProxyModel)
  KSelectionProxyModel *q_ptr;

  QItemSelectionModel *m_selectionModel;
  QList<QPersistentModelIndex> m_rootIndexList;

  QList<QAbstractProxyModel *> m_proxyChain;

  void sourceRowsAboutToBeInserted(const QModelIndex &, int start, int end);
  void sourceRowsInserted(const QModelIndex &, int start, int end);
  void sourceRowsAboutToBeRemoved(const QModelIndex &, int start, int end);
  void sourceRowsRemoved(const QModelIndex &, int start, int end);
  void sourceRowsAboutToBeMoved(const QModelIndex &parent, int start, int end, const QModelIndex &destParent, int destRow);
  void sourceRowsMoved(const QModelIndex &parent, int start, int end, const QModelIndex &destParent, int destRow);
  void sourceModelAboutToBeReset();
  void sourceModelReset();
  void sourceLayoutAboutToBeChanged();
  void sourceLayoutChanged();
  void sourceDataChanged(const QModelIndex &,const QModelIndex &);

  void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected );

  QModelIndexList toNonPersistent(const QList<QPersistentModelIndex> &list) const;

  /**
    Return true if @p idx is a descendant of one of the indexes in @p list.
    Note that this returns false if @p list contains @p idx.
  */
  bool isDescendantOf(QModelIndexList &list, const QModelIndex &idx) const;

  bool isDescendantOf(const QModelIndex &ancestor, const QModelIndex &descendant) const;

  QModelIndex childOfParent(const QModelIndex &ancestor, const QModelIndex &descendant) const;

  /**
    Returns the range in the proxy model corresponding to the range in the source model
    covered by @sourceParent, @p start and @p end.
  */
  QPair<int, int> getRootRange(const QModelIndex &sourceParent, int start, int end) const;

  /**
  Traverses the proxy models between the selectionModel and the sourceModel. Creating a chain as it goes.
  */
  void createProxyChain();

  /**
  When items are inserted or removed in the m_startWithChildTrees configuration,
  this method helps find the startRow for use emitting the signals from the proxy.
  */
  int getProxyInitialRow(const QModelIndex &parent) const;

  /**
  Returns a selection in which no descendants of selected indexes are also themselves selected.
  For example,
  @code
    A
    - B
    C
    D
  @endcode
  If A, B and D are selected in @p selection, the returned selection contains only A and D.
  */
  QItemSelection getRootRanges(const QItemSelection &selection) const;

  /**
    Returns the indexes in @p selection which are not already part of the proxy model.
  */
  QModelIndexList getNewIndexes(const QItemSelection &selection) const;

  /**
    Determines the correct location to insert @p index into @p list.
  */
  int getRootListRow(const QModelIndexList &list, const QModelIndex &index) const;

  /**
  If m_startWithChildTrees is true, this method returns the row in the proxy model to insert newIndex
  items.

  This is a special case because the items above rootListRow in the list are not in the model, but
  their children are. Those children must be counted.

  If m_startWithChildTrees is false, this method returns @p rootListRow.
  */
  int getTargetRow(int rootListRow);

  /**
    Regroups @p list into contiguous groups with the same parent.
  */
  QList<QPair<QModelIndex, QModelIndexList> > regroup(const QModelIndexList &list) const;

  /**
    Inserts the indexes in @p list into the proxy model.
  */
  void insertionSort(const QModelIndexList &list);

  /**
    Returns true if @p sourceIndex or one of its ascendants is already part of the proxy model.
  */
  bool isInModel(const QModelIndex &sourceIndex) const;

  /**
  Converts an index in the selection model to an index in the source model.
  */
  QModelIndex selectionIndexToSourceIndex(const QModelIndex &index) const;

  /**
    Returns the total number of children (but not descendants) of all of the indexes in @p list.
  */
  int childrenCount(const QModelIndexList &list) const;

  // Used to map children of indexes in the source model to indexes in the proxy model.
  // TODO: Find out if this breaks when indexes are modified because of higher siblings move/insert/remove
  mutable QHash< void *, QPersistentModelIndex> m_map;

  bool m_omitChildren;
  bool m_startWithChildTrees;
  bool m_omitDescendants;
  bool m_includeAllSelected;
  bool m_rowsRemoved;

  KSelectionProxyModel::FilterBehavior m_filterBehavior;

};

QModelIndexList KSelectionProxyModelPrivate::toNonPersistent(const QList<QPersistentModelIndex> &list) const
{
  QModelIndexList returnList;
  QList<QPersistentModelIndex>::const_iterator it;
  for (it = list.constBegin(); it != list.constEnd(); ++it)
    returnList << *it;

  return returnList;
}

void KSelectionProxyModelPrivate::sourceDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
  Q_Q(KSelectionProxyModel);

  QModelIndexList list = toNonPersistent(m_rootIndexList);
  if (!m_rootIndexList.contains(topLeft) && isInModel(topLeft))
  {
    // The easy case. A contiguous block not at the root of our model.
    QModelIndex proxyTopLeft = q->mapFromSource(topLeft);
    QModelIndex proxyBottomRight = q->mapFromSource(bottomRight);
    // If we're showing only chilren in our model and a grandchild is
    // changed, this will be invalid.
    if (!proxyTopLeft.isValid())
      return;
    emit q->dataChanged(proxyTopLeft, proxyBottomRight);
    return;
  }

  // We're not showing the m_rootIndexList, so we don't care if they change.
  if (m_startWithChildTrees)
    return;

  // The harder case. Parts of the reported changed range are part of
  // the model if they are in m_rootIndexList. Emit signals in blocks.

  const int leftColumn = topLeft.column();
  const int rightColumn = bottomRight.column();
  const QModelIndex parent = topLeft.parent();
  int startRow = topLeft.row();
  for (int row = startRow; row <= bottomRight.row(); ++row)
  {
    QModelIndex idx = q->sourceModel()->index(row, leftColumn, parent);
    if (m_rootIndexList.contains(idx))
    {
      startRow = row;
      ++row;
      idx = q->sourceModel()->index(row, leftColumn, parent);
      while(m_rootIndexList.contains(idx))
      {
        ++row;
        idx = q->sourceModel()->index(row, leftColumn, parent);
      }
      --row;
      QModelIndex sourceTopLeft = q->sourceModel()->index(startRow, leftColumn, parent);
      QModelIndex sourceBottomRight = q->sourceModel()->index(row, rightColumn, parent);
      QModelIndex proxyTopLeft = q->mapFromSource(sourceTopLeft);
      QModelIndex proxyBottomRight = q->mapFromSource(sourceBottomRight);

      emit q->dataChanged(proxyTopLeft, proxyBottomRight);
    }
  }
}


void KSelectionProxyModelPrivate::sourceLayoutAboutToBeChanged()
{
  Q_Q(KSelectionProxyModel);
  emit q->layoutAboutToBeChanged();
}

void KSelectionProxyModelPrivate::sourceLayoutChanged()
{
  Q_Q(KSelectionProxyModel);
  emit q->layoutChanged();
}

void KSelectionProxyModelPrivate::sourceModelAboutToBeReset()
{
  Q_Q(KSelectionProxyModel);
  // TODO: Uncomment for Qt 4.6
//   q->beginResetModel();
}

void KSelectionProxyModelPrivate::sourceModelReset()
{
  Q_Q(KSelectionProxyModel);

  // No need to try to refill this. When the model is reset it doesn't have a meaningful selection anymore,
  // but when it gets one we'll be notified anyway.
  m_rootIndexList.clear();
  // TODO: Uncomment for Qt 4.6
//   q->endResetModel();
  q->reset();
}

QPair<int, int> KSelectionProxyModelPrivate::getRootRange(const QModelIndex &sourceParent, int start, int end) const
{
  int listStart = -1;
  int listEnd = -1;

  int tracker = 0;
  foreach(const QModelIndex &idx, m_rootIndexList)
  {
    if (listStart == -1)
    {
      if (idx.row() > start && idx.parent() == sourceParent)
      {
        listStart = tracker;
      }
    }
    if (idx.row() < end && m_rootIndexList.value(tracker -1).parent() == sourceParent)
    {
      listEnd = tracker -1;
      break;
    }
    tracker++;

  }
  return qMakePair(listStart, listEnd);
}

int KSelectionProxyModelPrivate::getProxyInitialRow(const QModelIndex &parent) const
{
  Q_Q(const KSelectionProxyModel);
  int parentPosition = m_rootIndexList.indexOf(parent);

  QModelIndex parentAbove;
  while (parentPosition > 0)
  {
    parentPosition--;

    parentAbove = m_rootIndexList.at(parentPosition);

    int rows = q->sourceModel()->rowCount(parentAbove);
    if ( rows > 0 )
    {
      QModelIndex proxyChildAbove = q->mapFromSource(q->sourceModel()->index(rows, 0, parentAbove));
      return proxyChildAbove.row() + 1;
    }
  }
  return 0;
}

void KSelectionProxyModelPrivate::sourceRowsAboutToBeInserted(const QModelIndex &parent, int start, int end)
{
  Q_Q(KSelectionProxyModel);

  if (isInModel(parent) && !(m_startWithChildTrees && m_omitDescendants))
  {
    // The easy case.
    q->beginInsertRows(q->mapFromSource(parent), start, end);
    return;
  }

  if (m_startWithChildTrees && m_rootIndexList.contains(parent))
  {
    // Another fairly easy case.
    // The children of the indexes in m_rootIndexList are in the model, but their parents
    // are not, so we can't simply mapFromSource(parent) and get the row() to figure out
    // where the new rows are supposed to go.
    // Instead, we look at the 'higher' siblings of parent for their child items.
    // The lowest child item becomes the closest sibling of the new items.

    // sourceModel:
    // A
    // -> B
    // C
    // -> D
    // E
    // F

    // A, C, E and F are selected, then proxyModel is:
    //
    // B
    // D

    // If F gets a new child item, it must be inserted into the proxy model below D.
    // To find out what the proxyRow is, we find the proxyRow of D which is already in the model,
    // and +1 it.

    int proxyStartRow = getProxyInitialRow(parent);

    proxyStartRow += start;

    q->beginInsertRows(QModelIndex(), proxyStartRow, proxyStartRow + (end - start));
    return;
  }

}

void KSelectionProxyModelPrivate::sourceRowsInserted(const QModelIndex &parent, int start, int end)
{
  Q_Q(KSelectionProxyModel);
  Q_UNUSED(end);
  Q_UNUSED(start);

  if (isInModel(parent) && !(m_startWithChildTrees && m_omitDescendants))
  {
    q->endInsertRows();
    return;
  }

  if (m_startWithChildTrees && m_rootIndexList.contains(parent))
  {
    q->endInsertRows();
    return;
  }
}

void KSelectionProxyModelPrivate::sourceRowsAboutToBeRemoved(const QModelIndex &parent, int start, int end)
{
  Q_Q(KSelectionProxyModel);

  QModelIndex firstAffectedIndex = q->sourceModel()->index(start, 0, parent);

  QModelIndex proxyParent = q->mapFromSource(parent);
  QModelIndex proxyFirstAffectedIndex = q->mapFromSource(firstAffectedIndex);
  if ( proxyParent.isValid() )
  {
    // Get the easy case out of the way first.
    if (proxyFirstAffectedIndex.isValid())
    {
      m_rowsRemoved = true;
      q->beginRemoveRows(proxyParent, start, end);
      return;
    }
  }

  QModelIndexList removedSourceIndexes;
  removedSourceIndexes << firstAffectedIndex;
  for (int row = start + 1; row <= end; row++)
  {
    QModelIndex sourceChildIndex = q->sourceModel()->index(row, 0, parent);
    removedSourceIndexes << sourceChildIndex;
  }

  int proxyStart = -1;
  int proxyEnd = -1;

  // If we are going to remove a root index and all of its descendants, we need to start
  // at the top-most affected one.
  for (int row = 0; row < m_rootIndexList.size(); ++row)
  {
    QModelIndex rootIndex = m_rootIndexList.at(row);
    if (isDescendantOf(removedSourceIndexes, rootIndex) || removedSourceIndexes.contains(rootIndex))
    {
      proxyStart = row;
      break;
    }
  }

  // proxyEnd points to the last affected index in m_rootIndexList affected by the removal.
  for (int row = m_rootIndexList.size() - 1; row >= 0; --row)
  {
    QModelIndex rootIndex = m_rootIndexList.at(row);

    if (isDescendantOf(removedSourceIndexes, rootIndex) || removedSourceIndexes.contains(rootIndex))
    {
      proxyEnd = row;
      break;
    }
  }

  if (proxyStart == -1)
  {
    if (!m_startWithChildTrees)
    {
      return;
    }
    // No index in m_rootIndexList was a descendant of a deleted row.
    // Probably children of an index in m_rootIndex are being removed.
    if (!m_rootIndexList.contains(parent))
    {
      return;
    }

    int parentPosition = -1;
    if (!parent.isValid())
    {
      proxyStart = start;
    } else {
      parentPosition = m_rootIndexList.indexOf(parent);
      proxyStart = getTargetRow(parentPosition) + start;
    }

    proxyEnd = proxyStart + (end - start);

    // Descendants of children that are being removed must also be removed if they are also selected.
    for (int position = parentPosition + 1; position < m_rootIndexList.size(); ++position)
    {
      QModelIndex nextParent = m_rootIndexList.at(position);
      if (isDescendantOf(parent, nextParent))
      {
        if (end > childOfParent(parent, nextParent).row())
        {
          // All descendants of rows to be removed are accounted for.
          break;
        }

        proxyEnd += q->sourceModel()->rowCount(nextParent);
      } else {
        break;
      }
    }
  } else {
    if (m_startWithChildTrees)
    {
      proxyStart = getTargetRow(proxyStart);

      QModelIndex lastAffectedSourceParent = m_rootIndexList.at(proxyEnd);
      QModelIndex lastAffectedSourceChild = q->sourceModel()->index(q->sourceModel()->rowCount(lastAffectedSourceParent) - 1, 0, lastAffectedSourceParent);

      QModelIndex lastAffectedProxyChild = q->mapFromSource(lastAffectedSourceChild);

      proxyEnd = lastAffectedProxyChild.row();
    }

  }

  if (proxyStart == -1 || proxyEnd == -1)
    return;

  m_rowsRemoved = true;
  q->beginRemoveRows(QModelIndex(), proxyStart, proxyEnd);

}

void KSelectionProxyModelPrivate::sourceRowsRemoved(const QModelIndex &parent, int start, int end)
{
  Q_Q(KSelectionProxyModel);
  Q_UNUSED(parent)
  Q_UNUSED(start)
  Q_UNUSED(end)

  QMutableListIterator<QPersistentModelIndex> it(m_rootIndexList);
  while (it.hasNext())
  {
    QPersistentModelIndex idx = it.next();
    if (!idx.isValid())
    {
      it.remove();
    }
  }

  if (m_rowsRemoved)
    q->endRemoveRows();
  m_rowsRemoved = false;

}

void KSelectionProxyModelPrivate::sourceRowsAboutToBeMoved(const QModelIndex &srcParent, int srcStart, int srcEnd, const QModelIndex &destParent, int destRow)
{

  Q_Q(KSelectionProxyModel);

  if (isInModel(srcParent))
  {
    if (isInModel(destParent))
    {
      // The easy case.
      // TODO: Uncomment for Qt 4.6
//       q->beginMoveRows(q->mapFromSource(srcParent), srcStart, srcEnd, q->mapFromSource(destParent), destRow);
      return;
    }
  }
}

void KSelectionProxyModelPrivate::sourceRowsMoved(const QModelIndex &srcParent, int srcStart, int srcEnd, const QModelIndex &destParent, int destRow)
{
  Q_Q(KSelectionProxyModel);

  if (isInModel(srcParent))
  {
    if (isInModel(destParent))
    {
      // The easy case.
      // TODO: Uncomment for Qt 4.6
//       q->endMoveRows();
      return;
    }
  }
}

bool KSelectionProxyModelPrivate::isDescendantOf(QModelIndexList &list, const QModelIndex &idx) const
{
  QModelIndex parent = idx.parent();
  while (parent.isValid())
  {
    if (list.contains(parent))
      return true;
    parent = parent.parent();
  }
  return false;
}

bool KSelectionProxyModelPrivate::isDescendantOf(const QModelIndex &ancestor, const QModelIndex &descendant) const
{

  if (!descendant.isValid())
    return false;

  if (ancestor.isValid())
    return true;

  if (ancestor == descendant)
    return false;

  QModelIndex parent = descendant.parent();
  while (parent.isValid())
  {
    if (parent == ancestor)
      return true;

    parent = parent.parent();
  }
  return false;
}

QModelIndex KSelectionProxyModelPrivate::childOfParent(const QModelIndex& ancestor, const QModelIndex& descendant) const
{
//   if (ancestor = )
  QModelIndex parent = descendant.parent();
  while (parent != ancestor)
  {
    parent = parent.parent();
  }
  return parent;
}


QModelIndexList KSelectionProxyModelPrivate::getNewIndexes(const QItemSelection &selection) const
{
  QModelIndexList indexes;
  const int column = 0;

  foreach( const QItemSelectionRange &range, selection )
  {
    QModelIndex newIndex = range.topLeft();

    if (newIndex.column() != 0)
      continue;

    for(int row = newIndex.row(); row <= range.bottom(); ++row)
    {
      newIndex = newIndex.sibling(row, column);

      QModelIndex sourceNewIndex = selectionIndexToSourceIndex(newIndex);

      Q_ASSERT(sourceNewIndex.isValid());

      int startRow = m_rootIndexList.indexOf(sourceNewIndex);
      if ( startRow > 0 )
      {
        continue;
      }

      indexes << sourceNewIndex;
    }
  }
  return indexes;
}

void KSelectionProxyModelPrivate::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected )
{
  Q_Q(KSelectionProxyModel);

  // Any deselected indexes in the m_rootIndexList are removed. Then, any
  // indexes in the selected range which are not a descendant of one of the already selected indexes
  // are inserted into the model.
  //
  // All ranges from the selection model need to be split into individual rows. Ranges which are contiguous in
  // the selection model may not be contiguous in the source model if there's a sort filter proxy model in the chain.
  //
  // Some descendants of deselected indexes may still be selected. The ranges in m_selectionModel->selection()
  // are examined. If any of the ranges are descendants of one of the
  // indexes in deselected, they are added to the ranges to be inserted into the model.
  //
  // The new indexes are inserted in sorted order.

  QModelIndexList removeIndexes;

  const int column = 0;
  foreach( const QItemSelectionRange &range, deselected )
  {
    QModelIndex removeIndex = range.topLeft();

    if (removeIndex.column() != 0)
      continue;

    for(int row = removeIndex.row(); row <= range.bottom(); ++row)
    {
      removeIndex = removeIndex.sibling(row, column);

      removeIndexes << removeIndex;

      QModelIndex sourceRemoveIndex = selectionIndexToSourceIndex(removeIndex);

      int startRow = m_rootIndexList.indexOf(sourceRemoveIndex);

      if ( startRow < 0 )
        continue;

      if(m_startWithChildTrees)
      {
        int _start = 0;
        for (int i = 0 ; i < startRow; ++i)
        {
          _start += q->sourceModel()->rowCount(m_rootIndexList.at(i));
        }
        int rowCount = q->sourceModel()->rowCount(m_rootIndexList.at(startRow));
        if (rowCount <= 0)
        {
          // It doesn't have any children in the model, but we need to remove it from storage anyway.
          m_rootIndexList.removeAt(startRow);
          continue;
        }

        q->beginRemoveRows(QModelIndex(), _start, _start + rowCount - 1);
        m_rootIndexList.removeAt(startRow);
        q->endRemoveRows();
      } else {
        q->beginRemoveRows(QModelIndex(), startRow, startRow);
        m_rootIndexList.removeAt(startRow);
        q->endRemoveRows();
      }
    }
  }

  QItemSelection newRanges = getRootRanges(selected);

  if (!m_includeAllSelected)
  {
    QMutableListIterator<QItemSelectionRange> i(newRanges);
    while (i.hasNext())
    {
      QItemSelectionRange range = i.next();
      QModelIndex topLeft = range.topLeft();
      QModelIndexList list = toNonPersistent(m_rootIndexList);
      if (isDescendantOf(list, topLeft))
      {
        i.remove();
      }
    }
  }

  QModelIndexList newIndexes;

  newIndexes << getNewIndexes(newRanges);

  QItemSelection additionalRanges;
  if ( !m_includeAllSelected )
  {
    foreach( const QItemSelectionRange &range, m_selectionModel->selection() )
    {
      QModelIndex topLeft = range.topLeft();
      if (isDescendantOf(removeIndexes, topLeft))
      {
        QModelIndexList list = toNonPersistent(m_rootIndexList);
        if ( !isDescendantOf(list, topLeft) && !isDescendantOf(newIndexes, topLeft) )
          additionalRanges << range;
      }

      int row = m_rootIndexList.indexOf(topLeft);
      if (row >= 0)
      {
        if (isDescendantOf(newIndexes, topLeft))
        {
          q->beginRemoveRows(QModelIndex(), row, row);
          m_rootIndexList.removeAt(row);
          q->endRemoveRows();
        }
      }
    }
    // A
    // - B
    // - - C
    // - - D
    // - E
    // If A, B, C and E are selected, and A is deselected, B and E need to be inserted into the model, but not C.
    // Currently B, C and E are in additionalRanges. Ranges which are descendant of other ranges in the list need
    // to be removed.

    additionalRanges = getRootRanges(additionalRanges);

  }

  // TODO: Use QSet<QModelIndex> instead to simplify this stuff.
  foreach(const QModelIndex &idx, getNewIndexes(additionalRanges))
  {
    Q_ASSERT(idx.isValid());
    if (!newIndexes.contains(idx))
      newIndexes << idx;
  }
  if (newIndexes.size() > 0)
    insertionSort(newIndexes);
}

int KSelectionProxyModelPrivate::getRootListRow(const QModelIndexList &list, const QModelIndex &index) const
{

  if (list.isEmpty())
    return 0;

  // What's going on?
  // Consider a tree like
  //
  // A
  // - B
  // - - C
  // - - - D
  // - E
  // - F
  // - - G
  // - - - H
  // - I
  // - - J
  // - K
  //
  // If D, E and J are already selected, and H is newly selected, we need to put H between E and J in the proxy model.
  // To figure that out, we create a list for each already selected index of its ancestors. Then,
  // we climb the ancestors of H until we reach an index with siblings which have a descendant
  // selected (F above has siblings B, E and I which have descendants which are already selected).
  // Those child indexes are traversed to find the right sibling to put F beside.
  //
  // i.e., new items are inserted in the expected location.


  QList<QModelIndexList> rootAncestors;
  foreach(const QModelIndex &root, list)
  {
    QModelIndexList ancestors;
    ancestors << root;
    QModelIndex parent = root.parent();
    while (parent.isValid())
    {
      ancestors.prepend(parent);
      parent = parent.parent();
    }
    ancestors.prepend(QModelIndex());
    rootAncestors << ancestors;
  }

  QModelIndex commonParent = index;
  QModelIndex youngestAncestor;

  int firstCommonParent = -1;
  int bestParentRow = -1;
  while (commonParent.isValid())
  {
    youngestAncestor = commonParent;
    commonParent = commonParent.parent();

    for (int i = 0; i < rootAncestors.size(); ++i )
    {
      QModelIndexList ancestorList = rootAncestors.at(i);

      int parentRow = ancestorList.indexOf(commonParent);

      if (parentRow < 0)
        continue;

      if (parentRow > bestParentRow)
      {
        firstCommonParent = i;
        bestParentRow = parentRow;
      }
    }

    if (firstCommonParent >= 0)
      break;
  }

  // If @p list is non-empty, the invalid QModelIndex() will at least be found in ancestorList.
  Q_ASSERT(firstCommonParent >= 0);

  QModelIndexList firstAnsList = rootAncestors.at(firstCommonParent);

  QModelIndex eldestSibling = firstAnsList.value( bestParentRow + 1 );

  if (eldestSibling.isValid())
  {
    // firstCommonParent is a sibling of one of the ancestors of @p index.
    // It is the first index to share a common parent with one of the ancestors of @p index.
    if (eldestSibling.row() >= youngestAncestor.row())
      return firstCommonParent;
  }

  int siblingOffset = 1;

  // The same commonParent might be common to several root indexes.
  // If this is the last in the list, it's the only match. We instruct the model
  // to insert the new index after it ( + siblingOffset).
  if (rootAncestors.size() <= firstCommonParent + siblingOffset )
  {
    return firstCommonParent + siblingOffset;
  }

  // A
  // - B
  //   - C
  //   - D
  //   - E
  // F
  //
  // F is selected, then C then D. When inserting D into the model, the commonParent is B (the parent of C).
  // The next existing sibling of B is F (in the proxy model). bestParentRow will then refer to an index on
  // the level of a child of F (which doesn't exist - Boom!). If it doesn't exist, then we've already found
  // the place to insert D
  QModelIndexList ansList = rootAncestors.at(firstCommonParent + siblingOffset );
  if (ansList.size() <= bestParentRow)
  {
    return firstCommonParent + siblingOffset;
  }

  QModelIndex nextParent = ansList.at(bestParentRow);
  while (nextParent == commonParent)
  {
    if (ansList.size() < bestParentRow + 1)
      // If the list is longer, it means that at the end of it is a descendant of the new index.
      // We insert the ancestors items first in that case.
      break;

    QModelIndex nextSibling = ansList.value(bestParentRow + 1);

    if (!nextSibling.isValid())
    {
      continue;
    }

    if (youngestAncestor.row() <= nextSibling.row())
    {
      break;
    }

    siblingOffset++;

    if (rootAncestors.size() <= firstCommonParent + siblingOffset )
      break;

    ansList = rootAncestors.at(firstCommonParent + siblingOffset );

    // In the scenario above, E is selected after D, causing this loop to be entered,
    // and requiring a similar result if the next sibling in the proxy model does not have children.
    if (ansList.size() <= bestParentRow)
    {
      break;
    }

    nextParent = ansList.at(bestParentRow);
  }

  return firstCommonParent + siblingOffset;
}

QList<QPair<QModelIndex, QModelIndexList> > KSelectionProxyModelPrivate::regroup(const QModelIndexList &list) const
{
  QList<QPair<QModelIndex, QModelIndexList> > groups;

  // TODO: implement me.
//   foreach (const QModelIndex idx, list)
//   {
//
//   }

  return groups;
}

int KSelectionProxyModelPrivate::getTargetRow(int rootListRow)
{
  Q_Q(KSelectionProxyModel);
  if (!m_startWithChildTrees)
    return rootListRow;

  const int column = 0;

  --rootListRow;
  while (rootListRow >= 0)
  {
    QModelIndex idx = m_rootIndexList.at(rootListRow);
    Q_ASSERT(idx.isValid());
    int rowCount = q->sourceModel()->rowCount(idx);
    if (rowCount > 0)
    {

      QModelIndex proxyLastChild = q->mapFromSource(q->sourceModel()->index(rowCount - 1, column, idx));

      return proxyLastChild.row() + 1;
    }
    --rootListRow;
  }

  return 0;

}

void KSelectionProxyModelPrivate::insertionSort(const QModelIndexList &list)
{
  Q_Q(KSelectionProxyModel);

  // TODO: regroup indexes in list into contiguous ranges with the same parent.
//   QList<QPair<QModelIndex, QModelIndexList> > regroup(list);
  // where pair.first is a parent, and pair.second is a list of contiguous indexes of that parent.
  // That will allow emitting new rows in ranges rather than one at a time.

  foreach(const QModelIndex &newIndex, list)
  {
    if ( m_startWithChildTrees )
    {
      QModelIndexList list = toNonPersistent(m_rootIndexList);
      int rootListRow = getRootListRow(list, newIndex);
      int rowCount = q->sourceModel()->rowCount(newIndex);
      if ( rowCount > 0 )
      {
        int startRow = getTargetRow(rootListRow);
        q->beginInsertRows(QModelIndex(), startRow, startRow + rowCount - 1);
        Q_ASSERT(newIndex.isValid());
        m_rootIndexList.insert(rootListRow, newIndex);
        q->endInsertRows();
      } else {
        // Even if the newindex doesn't have any children to put into the model yet,
        // We still need to make sure it's future children are inserted into the model.
        m_rootIndexList.insert(rootListRow, newIndex);
      }
    } else {
      QModelIndexList list = toNonPersistent(m_rootIndexList);
      int row = getRootListRow(list, newIndex);
      q->beginInsertRows(QModelIndex(), row, row);
      Q_ASSERT(newIndex.isValid());
      m_rootIndexList.insert(row, newIndex);
      q->endInsertRows();
    }
  }
  return;
}

void KSelectionProxyModelPrivate::createProxyChain()
{
  Q_Q(KSelectionProxyModel);

  QAbstractItemModel *model = const_cast<QAbstractItemModel *>(m_selectionModel->model());
  QAbstractProxyModel *nextProxyModel;
  QAbstractProxyModel *proxyModel = qobject_cast<QAbstractProxyModel*>(model);

  QAbstractItemModel *rootModel;
  while (proxyModel)
  {

    if (proxyModel == q->sourceModel())
      break;

    m_proxyChain << proxyModel;

    nextProxyModel = qobject_cast<QAbstractProxyModel*>(proxyModel->sourceModel());

    if (!nextProxyModel)
    {
      rootModel = qobject_cast<QAbstractItemModel*>(proxyModel->sourceModel());
      // It's the final model in the chain, so it is necessarily the sourceModel.
      Q_ASSERT(rootModel == q->sourceModel());
      break;
    }
    proxyModel = nextProxyModel;
  }
}

QItemSelection KSelectionProxyModelPrivate::getRootRanges(const QItemSelection &selection) const
{
  QModelIndexList parents;
  QItemSelection rootSelection;
  QListIterator<QItemSelectionRange> i(selection);
  while (i.hasNext())
  {
    parents << i.next().topLeft();
  }

  i.toFront();

  while (i.hasNext())
  {
    QItemSelectionRange range = i.next();
    if (isDescendantOf(parents, range.topLeft()))
      continue;
    rootSelection << range;
  }
  return rootSelection;
}

QModelIndex KSelectionProxyModelPrivate::selectionIndexToSourceIndex(const QModelIndex &index) const
{
  QModelIndex seekIndex = index;
  QListIterator<QAbstractProxyModel*> i(m_proxyChain);
  QAbstractProxyModel *proxy;
  while (i.hasNext())
  {
    proxy = i.next();
    seekIndex = proxy->mapToSource(seekIndex);
  }
  return seekIndex;
}

bool KSelectionProxyModelPrivate::isInModel(const QModelIndex &sourceIndex) const
{
  if (m_rootIndexList.contains(sourceIndex))
  {
    if ( m_startWithChildTrees )
      return false;
    return true;
  }

  QModelIndex seekIndex = sourceIndex;
  while (seekIndex.isValid())
  {
    if (m_rootIndexList.contains(seekIndex))
    {
      return true;
    }

    seekIndex = seekIndex.parent();
  }
  return false;
}

KSelectionProxyModel::KSelectionProxyModel(QItemSelectionModel *selectionModel, QObject *parent)
  : QAbstractProxyModel(parent), d_ptr(new KSelectionProxyModelPrivate(this))
{
  Q_D(KSelectionProxyModel);

  d->m_selectionModel = selectionModel;

  connect( d->m_selectionModel, SIGNAL( selectionChanged( const QItemSelection &, const QItemSelection & ) ),
      SLOT( selectionChanged( const QItemSelection &, const QItemSelection & ) ) );

}

KSelectionProxyModel::~KSelectionProxyModel()
{
  delete d_ptr;
}

void KSelectionProxyModel::setFilterBehavior(FilterBehavior behavior)
{
  Q_D(KSelectionProxyModel);
  d->m_filterBehavior = behavior;

  switch(behavior)
  {
  case SelectedBranches:
  {
    d->m_omitChildren = false;
    d->m_omitDescendants = false;
    d->m_startWithChildTrees = false;
    d->m_includeAllSelected = false;
    break;
  }
  case SelectedBranchesRoots:
  {
    d->m_omitChildren = true;
    d->m_startWithChildTrees = false;
    d->m_includeAllSelected = false;
    break;
  }
  case SelectedBranchesChildren:
  {
    d->m_omitChildren = false;
    d->m_omitDescendants = true;
    d->m_startWithChildTrees = true;
    d->m_includeAllSelected = false;
    break;
  }
  case OnlySelected:
  {
    d->m_omitChildren = true;
    d->m_startWithChildTrees = false;
    d->m_includeAllSelected = true;
    break;
  }
  case OnlySelectedChildren:
  {
    d->m_omitChildren = false;
    d->m_omitDescendants = true;
    d->m_startWithChildTrees = true;
    d->m_includeAllSelected = true;
    break;
  }
  }
  reset();

}

KSelectionProxyModel::FilterBehavior KSelectionProxyModel::filterBehavior() const
{
  Q_D(const KSelectionProxyModel);
  return d->m_filterBehavior;
}

void KSelectionProxyModel::setSourceModel( QAbstractItemModel *sourceModel )
{
  Q_D(KSelectionProxyModel);

  QAbstractProxyModel::setSourceModel(sourceModel);
  d->createProxyChain();
  d->selectionChanged(d->m_selectionModel->selection(), QItemSelection());

  connect(sourceModel, SIGNAL(rowsAboutToBeInserted(const QModelIndex &, int, int)),
          SLOT(sourceRowsAboutToBeInserted(const QModelIndex &, int, int)));
  connect(sourceModel, SIGNAL(rowsInserted(const QModelIndex &, int, int)),
          SLOT(sourceRowsInserted(const QModelIndex &, int, int)));
  connect(sourceModel, SIGNAL(rowsAboutToBeRemoved(const QModelIndex &, int, int)),
          SLOT(sourceRowsAboutToBeRemoved(const QModelIndex &, int, int)));
  connect(sourceModel, SIGNAL(rowsRemoved(const QModelIndex &, int, int)),
          SLOT(sourceRowsRemoved(const QModelIndex &, int, int)));

  // TODO: Uncomment for Qt4.6
//   connect(sourceModel, SIGNAL(rowsAboutToBeMoved(const QModelIndex &, int, int, const QModelIndex &, int)),
//           SLOT(sourceRowsAboutToBeMoved(const QModelIndex &, int, int, const QModelIndex &, int)));
//   connect(sourceModel, SIGNAL(rowsMoved(const QModelIndex &, int, int, const QModelIndex &, int)),
//           SLOT(sourceRowsMoved(const QModelIndex &, int, int, const QModelIndex &, int)));
  connect(sourceModel, SIGNAL(modelAboutToBeReset()),
          SLOT(sourceModelAboutToBeReset()));
  connect(sourceModel, SIGNAL(modelReset()),
          SLOT(sourceModelReset()));
  connect(sourceModel, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)),
          SLOT(sourceDataChanged(const QModelIndex &, const QModelIndex & )));
  connect(sourceModel, SIGNAL(layoutAboutToBeChanged()),
          SLOT(sourceLayoutAboutToBeChanged()));
  connect(sourceModel, SIGNAL(layoutChanged()),
          SLOT(sourceLayoutChanged()));
}

QModelIndex KSelectionProxyModel::mapToSource(const QModelIndex &proxyIndex) const
{
  Q_D(const KSelectionProxyModel);

  if (!proxyIndex.isValid())
    return QModelIndex();

  QModelIndex idx = d->m_map.value(proxyIndex.internalPointer());
  idx = idx.sibling(idx.row(), proxyIndex.column());
  return idx;

}

QModelIndex KSelectionProxyModel::mapFromSource(const QModelIndex &sourceIndex) const
{
  Q_D(const KSelectionProxyModel);
  int row = d->m_rootIndexList.indexOf( sourceIndex );
  if ( row != -1 )
  {
    if ( !d->m_startWithChildTrees )
    {
      d->m_map.insert(sourceIndex.internalPointer(), QPersistentModelIndex(sourceIndex));
      return createIndex( row, sourceIndex.column(), sourceIndex.internalPointer() );
    }
    return QModelIndex();
  } else if ( d->isInModel( sourceIndex ) )
  {
    int targetRow = sourceIndex.row();
    if ( ( d->m_rootIndexList.contains( sourceIndex.parent() ) )
        && ( d->m_startWithChildTrees ) )
    {
      // loop over m_rootIndexList before sourceIndex, counting children.
      targetRow = 0;
      foreach(const QModelIndex &idx, d->m_rootIndexList)
      {
        if (idx == sourceIndex.parent())
          break;
        targetRow += sourceModel()->rowCount(idx);
      }
      targetRow += sourceIndex.row();
    }
    else if (d->m_startWithChildTrees)
      return QModelIndex();

    d->m_map.insert(sourceIndex.internalPointer(), QPersistentModelIndex(sourceIndex));
    return createIndex( targetRow, sourceIndex.column(), sourceIndex.internalPointer() );
  }
  return QModelIndex();
}

int KSelectionProxyModelPrivate::childrenCount(const QModelIndexList &list) const
{
  Q_Q(const KSelectionProxyModel);
  int count = 0;

  foreach(const QModelIndex &idx, list)
  {
    count += q->sourceModel()->rowCount(idx);
  }

  return count;
}

int KSelectionProxyModel::rowCount(const QModelIndex &index) const
{
  Q_D(const KSelectionProxyModel);

  if (!index.isValid())
  {
    if ( !d->m_startWithChildTrees )
    {
      return d->m_rootIndexList.size();
    } else {

      QModelIndexList list = d->toNonPersistent(d->m_rootIndexList);
      return d->childrenCount(list);
    }
  }

  if ( d->m_omitChildren )
    return 0;

  QModelIndex srcIndex = mapToSource(index);

  if (!d->isInModel(srcIndex))
    return 0;

  if ( d->m_omitDescendants )
  {
    if ( d->m_startWithChildTrees )
      return 0;

    if (d->m_rootIndexList.contains(srcIndex.parent()))
      return 0;
  }

  return sourceModel()->rowCount(srcIndex);
}

QModelIndex KSelectionProxyModel::index(int row, int column, const QModelIndex &parent) const
{
  Q_D(const KSelectionProxyModel);
  if (!hasIndex(row, column, parent))
    return QModelIndex();

  if (!parent.isValid())
  {
    if (!d->m_startWithChildTrees)
    {
      QModelIndex idx = d->m_rootIndexList.at( row );
      d->m_map.insert(idx.internalPointer(), idx);
      return createIndex(row, column, idx.internalPointer());
    }
    int _row = row;
    foreach(const QModelIndex &idx, d->m_rootIndexList)
    {
      int idxRowCount = sourceModel()->rowCount(idx);
      if ( _row < idxRowCount )
      {
        QModelIndex childIndex = sourceModel()->index(_row, column, idx);
        d->m_map.insert( childIndex.internalPointer(), QPersistentModelIndex( childIndex ) );
        return createIndex(row, childIndex.column(), childIndex.internalPointer());
      }
      _row -= idxRowCount;
    }

    return QModelIndex();
  } else
  {
    QModelIndex sourceParent = mapToSource(parent);
    QModelIndex sourceIndex = sourceModel()->index(row, column, sourceParent);
    return mapFromSource(sourceIndex);
  }
}

QModelIndex KSelectionProxyModel::parent(const QModelIndex &index) const
{
  Q_D(const KSelectionProxyModel);

  QModelIndex sourceIndex = mapToSource(index);
  if (d->m_rootIndexList.contains(sourceIndex.parent()) && ( d->m_startWithChildTrees || d->m_omitChildren ) )
  {
    return QModelIndex();
  }

  if (d->m_rootIndexList.contains(sourceIndex))
    return QModelIndex();

  QModelIndex proxyParent = mapFromSource(sourceIndex.parent());
  return proxyParent;
}

Qt::ItemFlags KSelectionProxyModel::flags( const QModelIndex &index ) const
{
  if (!index.isValid())
    return 0;

  QModelIndex srcIndex = mapToSource(index);
  return sourceModel()->flags(srcIndex);
}

QVariant KSelectionProxyModel::data( const QModelIndex & index, int role ) const
{
  if (index.isValid())
  {
    QModelIndex idx = mapToSource(index);
    return idx.data(role);
  }
  return sourceModel()->data(index, role);
}

QVariant KSelectionProxyModel::headerData( int section, Qt::Orientation orientation, int role  ) const
{
  return sourceModel()->headerData(section, orientation, role);
}

bool KSelectionProxyModel::hasChildren ( const QModelIndex & parent) const
{
  return rowCount(parent) > 0;
}

int KSelectionProxyModel::columnCount(const QModelIndex &index) const
{
  return sourceModel()->columnCount(mapToSource(index));
}

QItemSelectionModel *KSelectionProxyModel::selectionModel() const
{
  Q_D(const KSelectionProxyModel);
  return d->m_selectionModel;
}

#include "moc_kselectionproxymodel.cpp"
