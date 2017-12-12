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
#pragma once

#include <QAbstractItemModel>
#include <qmimedata.h>

#include <AzCore/EBus/EBus.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <GraphCanvas/Widgets/GraphCanvasMimeContainer.h>
#include <GraphCanvas/Widgets/GraphCanvasMimeEvent.h>
#include <GraphCanvas/Widgets/GraphCanvasTreeItem.h>

namespace GraphCanvas
{
    class GraphCanvasTreeModelRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = GraphCanvasTreeModel*;

        virtual void ClearSelection() = 0;
    };

    using GraphCanvasTreeModelRequestBus = AZ::EBus<GraphCanvasTreeModelRequests>;

    //! Contains all the information required to build any Tree based widget that will support Drag/Drop with the GraphicsView.
    class GraphCanvasTreeModel
        : public QAbstractItemModel
    {
    public:
        AZ_CLASS_ALLOCATOR(GraphCanvasTreeModel, AZ::SystemAllocator, 0);
        static void Reflect(AZ::ReflectContext* reflectContext);

        GraphCanvasTreeModel(GraphCanvasTreeItem* treeRoot, QObject* parent = nullptr)
            : QAbstractItemModel(parent)
            , m_treeRoot(treeRoot)
        {
        }

        ~GraphCanvasTreeModel() = default;
        
        QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override
        {
            if (!hasIndex(row, column, parent))
            {
                return QModelIndex();
            }

            GraphCanvasTreeItem* parentItem;

            if (!parent.isValid())
            {
                parentItem = m_treeRoot.get();
            }
            else
            {
                parentItem = static_cast<GraphCanvasTreeItem*>(parent.internalPointer());
            }

            GraphCanvasTreeItem* childItem = parentItem->ChildForRow(row);

            if (childItem)
            {
                QModelIndex modelIndex = createIndex(row, column, childItem);
                childItem->RegisterModel(const_cast<GraphCanvasTreeModel*>(this), modelIndex);
                return modelIndex;
            }
            else
            {
                return QModelIndex();
            }
        }

        QModelIndex parent(const QModelIndex& index) const override
        {
            if (!index.isValid())
            {
                return QModelIndex();
            }

            GraphCanvasTreeItem* childItem = static_cast<GraphCanvasTreeItem*>(index.internalPointer());
            GraphCanvasTreeItem* parentItem = childItem->GetParent();

            if ((!parentItem) || (parentItem == m_treeRoot.get()))
            {
                return QModelIndex();
            }

            QModelIndex modelIndex = createIndex(parentItem->FindRowUnderParent(), index.column(), parentItem);
            parentItem->RegisterModel(const_cast<GraphCanvasTreeModel*>(this), modelIndex);
            return modelIndex;
        }
        
        int columnCount(const QModelIndex& parent = QModelIndex()) const override
        {
            if (parent.isValid() && parent.internalPointer() != nullptr)
            {
                return static_cast<GraphCanvasTreeItem*>(parent.internalPointer())->GetColumnCount();
            }
            else
            {
                return m_treeRoot->GetColumnCount();
            }
        }

        int rowCount(const QModelIndex& parent = QModelIndex()) const override
        {
            GraphCanvasTreeItem* parentItem;

            if (parent.column() > 0)
            {
                return 0;
            }

            if (!parent.isValid())
            {
                parentItem = m_treeRoot.get();
            }
            else
            {
                parentItem = static_cast<GraphCanvasTreeItem*>(parent.internalPointer());
            }

            return parentItem->GetNumChildren();
        }

        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override
        {
            if (!index.isValid())
            {
                return QVariant();
            }

            GraphCanvasTreeItem* item = static_cast<GraphCanvasTreeItem*>(index.internalPointer());
            return item->Data(index, role);
        }

        Qt::ItemFlags flags(const QModelIndex& index) const override
        {
            if (!index.isValid())
            {
                return 0;
            }

            GraphCanvasTreeItem* item = static_cast<GraphCanvasTreeItem*>(index.internalPointer());
            return item->Flags(index);
        }

        void setMimeType(const char* mimeType)
        {
            m_mimeType = mimeType;
        }

        QStringList mimeTypes() const override
        {
            QStringList list;
            list.append(m_mimeType);
            return list;
        }

        QMimeData* mimeData(const QModelIndexList& indexes) const
        {
            if (m_mimeType.isEmpty())
            {
                return nullptr;
            }

            GraphCanvasMimeContainer container;
            for (const QModelIndex& index : indexes)
            {
                GraphCanvasTreeItem* item = static_cast<GraphCanvasTreeItem*>(index.internalPointer());
                GraphCanvasMimeEvent* mimeEvent = item->CreateMimeEvent();

                if (mimeEvent)
                {
                    container.m_mimeEvents.push_back(mimeEvent);
                }
            }

            if (container.m_mimeEvents.empty())
            {
                return nullptr;
            }

            AZStd::vector<char> encoded;
            if (!container.ToBuffer(encoded))
            {
                return nullptr;
            }

            QMimeData* mimeDataPtr = new QMimeData();
            QByteArray encodedData;
            encodedData.resize((int)encoded.size());
            memcpy(encodedData.data(), encoded.data(), encoded.size());
            mimeDataPtr->setData(m_mimeType, encodedData);

            return mimeDataPtr;
        }
        
        bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override
        {
            GraphCanvasTreeItem* parentItem = static_cast<GraphCanvasTreeItem*>(parent.internalPointer());

            if (parentItem == nullptr)
            {
                return false;
            }

            if (row > parentItem->m_childItems.size())
            {
                AZ_Error("Graph Canvas", false, "Trying to remove invalid row from GraphCanvasTreeModel.");
                return false;
            }
            else if (row + count > parentItem->m_childItems.size())
            {
                AZ_Warning("Graph Canvas", false, "Trying to remove too many rows from GraphCanvasTreeModel.");
                count = (static_cast<int>(parentItem->m_childItems.size()) - row);
            }

            if (count == 0)
            {
                return true;
            }

            GraphCanvasTreeModelRequestBus::Event(this, &GraphCanvasTreeModelRequests::ClearSelection);

            beginRemoveRows(parent, row, row + (count - 1));

            for (int i=0; i < count; ++i)
            {
                GraphCanvasTreeItem* childItem = parentItem->m_childItems[row + i];
                childItem->RemoveParent(parentItem);

                if (parentItem->m_deleteRemoveChildren)
                {
                    delete childItem;
                }
            }

            parentItem->m_childItems.erase(parentItem->m_childItems.begin() + row, parentItem->m_childItems.begin() + row + count);

            endRemoveRows();

            return true;
        }

    public:

        QString                                m_mimeType;
        AZStd::unique_ptr<GraphCanvasTreeItem> m_treeRoot;
    };
}
