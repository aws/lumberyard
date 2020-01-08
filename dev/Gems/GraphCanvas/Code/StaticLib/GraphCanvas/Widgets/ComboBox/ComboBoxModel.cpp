/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright EntityRef license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#include <QFont>

#include <GraphCanvas/Widgets/ComboBox/ComboBoxModel.h>

namespace GraphCanvas
{
    /////////////////////////////////
    // GraphCanvasListComboBoxModel
    /////////////////////////////////

    void GraphCanvasListComboBoxModel::SetFontScale(qreal fontScale)
    {
        m_fontScale = fontScale;

        layoutChanged();
    }

    QModelIndex GraphCanvasListComboBoxModel::AddElement(const AZStd::string& element)
    {
        beginInsertRows(QModelIndex(), static_cast<int>(m_elements.size()), static_cast<int>(m_elements.size()));
        m_elements.emplace_back(element.c_str());
        endInsertRows();

        return index(static_cast<int>(m_elements.size()) - 1, ColumnIndex::Name);
    }
    
    void GraphCanvasListComboBoxModel::RemoveElement(const AZStd::string& element)
    {
        RemoveElement(FindIndexForName(element.c_str()));
    }

    void GraphCanvasListComboBoxModel::RemoveElement(const QModelIndex& index)
    {
        if (index.isValid())
        {
            m_elements.erase(m_elements.begin() + index.row());
        }
    }

    int GraphCanvasListComboBoxModel::rowCount(const QModelIndex& parent) const
    {
        return static_cast<int>(m_elements.size());
    }

    QVariant GraphCanvasListComboBoxModel::data(const QModelIndex& index, int role) const
    {
        switch (role)
        {
        case Qt::DisplayRole:
        {
            return GetNameForIndex(index);
        }
        case Qt::FontRole:
        {
            QFont sizedFont;
            int pointSize = sizedFont.pointSize();
            sizedFont.setPointSizeF(pointSize * m_fontScale);
            return sizedFont;
        }
        default:
            break;
        }

        return GetRoleData(index, role);
    }

    QString GraphCanvasListComboBoxModel::GetNameForIndex(const QModelIndex& index) const
    {
        if (!index.isValid()
            || index.row() < 0 
            || index.row() >= m_elements.size())
        {
            return "";
        }

        return m_elements[index.row()];
    }

    QModelIndex GraphCanvasListComboBoxModel::FindIndexForName(const QString& name) const
    {
        for (int i = 0; i < m_elements.size(); ++i)
        {
            if (m_elements[i] == name)
            {
                return createIndex(i, ColumnIndex::Name, nullptr);
            }
        }

        return QModelIndex();
    }

    QModelIndex GraphCanvasListComboBoxModel::GetDefaultIndex() const
    {
        if (m_elements.empty())
        {
            return QModelIndex();
        }

        return createIndex(0, ColumnIndex::Name, nullptr);
    }

    QAbstractItemModel* GraphCanvasListComboBoxModel::GetDropDownItemModel()
    {
        return this;
    }

    int GraphCanvasListComboBoxModel::GetSortColumn() const
    {
        return ColumnIndex::Name;
    }

    int GraphCanvasListComboBoxModel::GetFilterColumn() const
    {
        return ColumnIndex::Name;
    }

    QModelIndex GraphCanvasListComboBoxModel::GetNextIndex(const QModelIndex& modelIndex) const
    {
        if (!modelIndex.isValid())
        {
            return QModelIndex();
        }

        int nextRow = modelIndex.row() + 1;        

        if (nextRow == rowCount(modelIndex.parent()))
        {
            nextRow = 0;
        }

        if (nextRow == rowCount(modelIndex.parent()))
        {
            return QModelIndex();
        }

        return index(nextRow, ColumnIndex::Name);
    }

    QModelIndex GraphCanvasListComboBoxModel::GetPreviousIndex(const QModelIndex& modelIndex) const
    {
        if (!modelIndex.isValid())
        {
            return QModelIndex();
        }

        int nextRow = modelIndex.row() - 1;

        if (nextRow < 0)
        {
            nextRow = rowCount(modelIndex.parent()) - 1;
        }

        if (nextRow < 0)
        {
            return QModelIndex();
        }

        return index(nextRow, ColumnIndex::Name);
    }

    QAbstractListModel* GraphCanvasListComboBoxModel::GetCompleterItemModel()
    {
        return this;
    }

    int GraphCanvasListComboBoxModel::GetCompleterColumn() const
    {
        return ColumnIndex::Name;
    }

    QVariant GraphCanvasListComboBoxModel::GetRoleData(const QModelIndex& index, int role) const
    {
        AZ_UNUSED(index);
        AZ_UNUSED(role);

        return QVariant();
    }
}

#include <StaticLib/GraphCanvas/Widgets/ComboBox/ComboBoxModel.moc>