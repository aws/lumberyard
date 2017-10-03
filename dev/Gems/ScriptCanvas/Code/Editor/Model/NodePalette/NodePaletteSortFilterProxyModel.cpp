/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "precompiled.h"
#include "NodePaletteSortFilterProxyModel.h"

#include <Editor/View/Widgets/NodePalette/NodePaletteTreeItem.h>

#include <QRegExp>

namespace ScriptCanvasEditor
{
    namespace Model
    {
        bool NodePaletteSortFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
        {
            QAbstractItemModel* model = sourceModel();
            QModelIndex index = model->index(sourceRow, 0, sourceParent);

            NodePaletteTreeItem* currentItem = static_cast<NodePaletteTreeItem*>(index.internalPointer());

            if (m_filter.isEmpty())
            {
                currentItem->ClearHighlight();
                return true;
            }

            QRegExp filterRegEx = QRegExp(m_filter, Qt::CaseInsensitive);
            QString test = model->data(index).toString();

            bool showRow = false;
            int regexIndex = test.lastIndexOf(filterRegEx);

            if (regexIndex >= 0)
            {
                showRow = true;

                AZStd::pair<int, int> highlight(regexIndex, m_filter.size());
                currentItem->SetHighlight(highlight);
            }
            else
            {
                currentItem->ClearHighlight();
            }

            // If we have children. We need to remain visible if any of them are valid.
            if (!showRow && sourceModel()->hasChildren(index))
            {
                for (int i = 0; i < sourceModel()->rowCount(index); ++i)
                {
                    if (filterAcceptsRow(i, index))
                    {
                        showRow = true;
                        break;
                    }
                }
            }

            // We also want to display ourselves if any of our parents match the filter
            QModelIndex parentIndex = sourceModel()->parent(index);
            while (!showRow && parentIndex.isValid())
            {
                QString test = model->data(parentIndex).toString();
                showRow = test.contains(filterRegEx);

                parentIndex = sourceModel()->parent(parentIndex);
            }

            return showRow;
        }

        #include <Editor/Model/NodePalette/NodePaletteSortFilterProxyModel.moc>
    }
}
