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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "pch.h"
#include <QFont>
#include <QIcon>
#include <QPalette>
#include <QMimeData>
#include "ExplorerModel.h"
#include "Explorer.h"
#include "AnimationList.h"
#include "Expected.h"

#if 0
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define TRACE_MODEL(format, ...)  { char buf[1024] = {}; sprintf_s(buf, "0x%p: " format "\n", this, __VA_ARGS__); OutputDebugString(buf); }
#else
#define TRACE_MODEL(...)
#endif


namespace CharacterTool
{
    ExplorerModel::ExplorerModel(Explorer* list, QObject* parent)
        : QAbstractItemModel(parent)
        , m_explorer(list)
        , m_rootIndex(0)
        , m_rootSubtree(NUM_SUBTREES)
        , m_addWithinActiveRoot(false)
        , m_removeWithinActiveRoot(false)
    {
        EXPECTED(connect(m_explorer, SIGNAL(SignalEntryModified(ExplorerEntryModifyEvent &)), this, SLOT(OnEntryModified(ExplorerEntryModifyEvent &))));
        EXPECTED(connect(m_explorer, SIGNAL(SignalBeginAddEntry(ExplorerEntry*)), this, SLOT(OnBeginAddEntry(ExplorerEntry*))));
        EXPECTED(connect(m_explorer, SIGNAL(SignalEndAddEntry()), this, SLOT(OnEndAddEntry())));
        EXPECTED(connect(m_explorer, SIGNAL(SignalBeginRemoveEntry(ExplorerEntry*)), this, SLOT(OnBeginRemoveEntry(ExplorerEntry*))));
        EXPECTED(connect(m_explorer, SIGNAL(SignalEndRemoveEntry()), this, SLOT(OnEndRemoveEntry())));
    }

    void ExplorerModel::SetRootByIndex(int index)
    {
        if (m_rootIndex != index)
        {
            beginResetModel();
            m_rootIndex = index;
            if (index == 0)
            {
                m_rootSubtree = NUM_SUBTREES;
            }
            else
            {
                m_rootSubtree = GetActiveRoot()->subtree;
            }
            endResetModel();
        }
    }

    ExplorerEntry* ExplorerModel::GetEntry(const QModelIndex& index)
    {
        return ((ExplorerEntry*)index.internalPointer());
    }

    ExplorerEntry* ExplorerModel::GetActiveRoot() const
    {
        if (m_rootIndex < 1 || m_rootIndex > m_explorer->GetRoot()->children.size())
        {
            return m_explorer->GetRoot();
        }
        else
        {
            return m_explorer->GetRoot()->children[m_rootIndex - 1];
        }
    }

    int ExplorerModel::GetRootIndex() const
    {
        return m_rootIndex;
    }


    QModelIndex ExplorerModel::index(int row, int column, const QModelIndex& parent) const
    {
        if (row < 0 || column < 0)
        {
            return QModelIndex();
        }
        ExplorerEntry* entry = GetEntry(parent);
        if (!entry)
        {
            entry = GetActiveRoot();
        }
        if (size_t(row) >= entry->children.size())
        {
            return QModelIndex();
        }
        return createIndex(row, column, entry->children[row]);
    }

    int ExplorerModel::rowCount(const QModelIndex& parent) const
    {
        if (parent.column() > 0)
        {
            return 0;
        }
        ExplorerEntry* entry = GetEntry(parent);
        if (!entry)
        {
            entry = GetActiveRoot();
        }
        return entry->children.size();
    }

    int ExplorerModel::columnCount(const QModelIndex& parent) const
    {
        return m_explorer->GetColumnCount();
    }

    QVariant ExplorerModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
        {
            return QString(m_explorer->GetColumnLabel(section));
        }
        else if (role == Qt::SizeHintRole && orientation == Qt::Horizontal)
        {
            if (section == 0)
            {
                return QSize(200, 20);
            }
            else
            {
                if (const ExplorerColumn* column = m_explorer->GetColumn(section))
                {
                    if (column->format == ExplorerColumn::FILESIZE)
                    {
                        return QSize(40, 20);
                    }
                    else if (column->format == ExplorerColumn::ICON)
                    {
                        return QSize(20, 20);
                    }
                }
                return QSize(50, 20);
            }
        }
        return QVariant();
    }

    bool ExplorerModel::hasChildren(const QModelIndex& parent) const
    {
        ExplorerEntry* entry = GetEntry(parent);
        if (!entry)
        {
            entry = GetActiveRoot();
        }
        return !entry->children.empty();
    }

    Qt::ItemFlags ExplorerModel::flags(const QModelIndex& index) const
    {
        Qt::ItemFlags flags = QAbstractItemModel::flags(index);

        if (index.isValid() && GetEntry(index)->isDragEnabled)
        {
            flags |= Qt::ItemIsDragEnabled;
        }

        return flags;
    }

    QVariant ExplorerModel::data(const QModelIndex& index, int role) const
    {
        if (!index.isValid())
        {
            return QVariant();
        }
        ExplorerEntry* entry = GetEntry(index);
        switch (role)
        {
        case Qt::DisplayRole:
        {
            if (entry)
            {
                if (index.column() == 0)
                {
                    if (entry->modified)
                    {
                        return QString("*") + QString(entry->name.c_str());
                    }
                    else
                    {
                        return QString(entry->name.c_str());
                    }
                }
                else
                {
                    const ExplorerColumn* column = m_explorer->GetColumn(index.column());
                    if (column && size_t(index.column()) < entry->columnValues.size())
                    {
                        if (column->format == ExplorerColumn::FILESIZE)
                        {
                            unsigned int value = entry->columnValues[index.column()];
                            if (value != 0)
                            {
                                QString result;
                                result.sprintf("%i KB", value / 1024);
                                return result;
                            }
                            else
                            {
                                return QVariant();
                            }
                        }
                        else if (column->format == ExplorerColumn::ICON)
                        {
                            return QVariant();
                        }
                        else
                        {
                            return QVariant(entry->columnValues[index.column()]);
                        }
                    }
                    else
                    {
                        return QVariant();
                    }
                }
            }
            return QVariant();
        }
        case Qt::FontRole:
        {
            if (entry)
            {
                if (entry->type == ENTRY_SUBTREE_ROOT)
                {
                    QFont font;
                    font.setBold(true);
                    return font;
                }
            }
            return QVariant();
        }
        case Qt::DecorationRole:
        {
            if (entry)
            {
                if (index.column() == 0)
                {
                    return QIcon(Explorer::IconForEntry(entry->type, entry));
                }
                else
                {
                    const ExplorerColumn* column = m_explorer->GetColumn(index.column());
                    if (column && size_t(index.column()) < entry->columnValues.size())
                    {
                        int iconIndex = entry->columnValues[index.column()];
                        if (iconIndex < column->values.size())
                        {
                            return QIcon(column->values[iconIndex].icon);
                        }
                        else
                        {
                            return QIcon();
                        }
                    }
                }
            }
            return QVariant();
        }
        case Qt::TextAlignmentRole:
        {
            if (index.column() == 0)
            {
                return QVariant();
            }
            else
            {
                return QVariant(Qt::AlignRight);
            }
        }
        case Qt::SizeHintRole:
        {
            int section = index.column();
            if (const ExplorerColumn* column = m_explorer->GetColumn(section))
            {
                if (column->format == ExplorerColumn::FILESIZE)
                {
                    return QSize(40, 20);
                }
                else if (column->format == ExplorerColumn::ICON)
                {
                    return QSize(20, 20);
                }
            }
            return QSize(50, 20);
        }
        case Qt::UserRole: // we use UserRole for sorting
        {
            if (index.column() == 0 && (m_rootIndex == 0 && index.parent() == QModelIndex()))
            {
                // compare first level by subtree index
                return QVariant(int(entry ? entry->subtree : 0));
            }
            else if (index.column() == 0)
            {
                // and nested items by name
                return QVariant(QString(entry->name.c_str()).toLower());
            }
            else
            {
                int column = index.column();
                if (size_t(column) < entry->columnValues.size())
                {
                    return QVariant((int)entry->columnValues[column]);
                }
                else
                {
                    return QVariant();
                }
            }
            return QVariant();
        }
        }
        return QVariant();
    }

    QModelIndex ExplorerModel::ModelIndexFromEntry(ExplorerEntry* entry, int column) const
    {
        if (entry == GetActiveRoot())
        {
            return QModelIndex();
        }
        if (!EXPECTED(entry != m_explorer->GetRoot()))
        {
            return QModelIndex();
        }
        if (!EXPECTED(entry != 0))
        {
            return QModelIndex();
        }
        if (entry->parent == 0) // FIXME
        {
            return QModelIndex();
        }

        for (size_t i = 0; i < entry->parent->children.size(); ++i)
        {
            if (entry->parent->children[i] == entry)
            {
                return createIndex(i, column, entry);
            }
        }

        CRY_ASSERT(0);
        return createIndex(0, column, entry);
    }

    QModelIndex ExplorerModel::NewModelIndexFromEntry(ExplorerEntry* entry, int column) const
    {
        if (entry == GetActiveRoot())
        {
            return QModelIndex();
        }
        if (!EXPECTED(entry != m_explorer->GetRoot()))
        {
            return QModelIndex();
        }

        int i = (int)entry->parent->children.size();
        return createIndex(i, column, entry);
    }

    QModelIndex ExplorerModel::parent(const QModelIndex& index) const
    {
        if (!index.isValid())
        {
            return QModelIndex();
        }
        ExplorerEntry* entry = GetEntry(index);
        if (entry == GetActiveRoot())
        {
            return QModelIndex();
        }
        if (!EXPECTED(entry != m_explorer->GetRoot()))
        {
            return QModelIndex();
        }

        ExplorerEntry* parent = entry->parent;
        if (!EXPECTED(parent != 0))
        {
            return QModelIndex();
        }
        if (parent == GetActiveRoot())
        {
            return QModelIndex();
        }
        if (!EXPECTED(parent != m_explorer->GetRoot()))
        {
            return QModelIndex();
        }
        return ModelIndexFromEntry(parent, 0);
    }

    QMimeData* ExplorerModel::mimeData(const QModelIndexList& indices) const
    {
        std::vector<ExplorerEntry*> entries;
        entries.reserve(indices.size());

        for (auto& index : indices)
        {
            if (index.isValid())
            {
                entries.push_back(GetEntry(index));
            }
        }

        return m_explorer->GetMimeDataForEntries(entries);
    }

    Qt::DropActions ExplorerModel::supportedDragActions() const
    {
        return Qt::CopyAction;
    }

    void ExplorerModel::OnEntryModified(ExplorerEntryModifyEvent& ev)
    {
        if (ev.continuousChange)
        {
            return;
        }
        if (m_rootIndex != 0 && ev.entry->subtree != m_rootSubtree)
        {
            return;
        }

        QModelIndex rangeStart = ModelIndexFromEntry(ev.entry, ev.entryParts & ENTRY_PART_CONTENT ? 0 : 1);
        QModelIndex rangeEnd = ModelIndexFromEntry(ev.entry, ev.entryParts & ENTRY_PART_STATUS_COLUMNS ? m_explorer->GetColumnCount() - 1 : 1);
        dataChanged(rangeStart, rangeEnd);
    }

    void ExplorerModel::OnBeginAddEntry(ExplorerEntry* entry)
    {
        if (!EXPECTED(entry->parent != 0))
        {
            return;
        }
        if (m_rootIndex != 0 && entry->subtree != m_rootSubtree)
        {
            m_addWithinActiveRoot = false;
            TRACE_MODEL("skip add: %s", entry->path.c_str());
        }
        else
        {
            m_addWithinActiveRoot = true;

            QModelIndex parent = entry == GetActiveRoot() ? QModelIndex() : ModelIndexFromEntry(entry->parent, 0);
            int newRowIndex = entry->parent->children.size();
            beginInsertRows(parent, newRowIndex, newRowIndex);
            TRACE_MODEL("add: %s", entry->path.c_str());
        }
    }

    void ExplorerModel::OnEndAddEntry()
    {
        if (m_addWithinActiveRoot)
        {
            endInsertRows();
        }
    }

    void ExplorerModel::OnBeginRemoveEntry(ExplorerEntry* entry)
    {
        if (m_rootIndex != 0 && entry->subtree != m_rootSubtree)
        {
            m_removeWithinActiveRoot = false;
            TRACE_MODEL("skip remove: %s", entry->path.c_str());
        }
        else
        {
            m_removeWithinActiveRoot = true;

            QModelIndex parent = ModelIndexFromEntry(entry->parent, 0);
            QModelIndex index = ModelIndexFromEntry(entry, 0);

            TRACE_MODEL("begin remove row: (%i, %i, 0x%p)", index.row(), index.column(), index.internalPointer());
            ExplorerEntry* removedEntry = GetEntry(index);
            EXPECTED(removedEntry == entry);
            beginRemoveRows(parent, index.row(), index.row());
            TRACE_MODEL("remove: %s", entry->path.c_str());
        }
    }


    void ExplorerModel::OnEndRemoveEntry()
    {
        if (m_removeWithinActiveRoot)
        {
            TRACE_MODEL("end remove row");
            endRemoveRows();
        }
    }
}

#include <CharacterTool/ExplorerModel.moc>
