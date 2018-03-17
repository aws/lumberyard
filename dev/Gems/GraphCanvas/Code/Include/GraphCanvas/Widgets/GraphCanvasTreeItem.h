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

#include <AzCore/std/functional_basic.h>

namespace GraphCanvas
{
    class GraphCanvasMimeEvent;
    class GraphCanvasTreeModel;
    
    class GraphCanvasTreeItem
    {
    private:
        struct Comparator
        {
            Comparator() = default;

            bool operator()(const GraphCanvasTreeItem* lhs, const GraphCanvasTreeItem* rhs) const
            {
                return lhs->LessThan(rhs);
            }
        };

        friend struct Comparator;
        friend class GraphCanvasTreeModel;
    public:
        AZ_CLASS_ALLOCATOR(GraphCanvasTreeItem, AZ::SystemAllocator, 0);

        virtual ~GraphCanvasTreeItem()
        {
            if (m_parent)
            {
                const bool deleteObject = false;
                m_parent->RemoveChild(this, deleteObject);
            }

            for (GraphCanvasTreeItem* item : m_childItems)
            {
                item->RemoveParent(this);
                delete item;
            }

            m_childItems.clear();
        }

        void DetachItem()
        {
            if (m_parent)
            {
                const bool deleteObject = false;
                m_parent->RemoveChild(this, deleteObject);
            }
        }
        
        int GetNumChildren() const
        {
            return static_cast<int>(m_childItems.size());
        }

        GraphCanvasTreeItem* ChildForRow(int row) const
        {
            GraphCanvasTreeItem* treeItem = nullptr;

            if (row >= 0 && row < m_childItems.size())
            {
                treeItem = m_childItems[row];
            }

            return treeItem;
        }

        int FindRowUnderParent() const
        {
            if (m_parent)
            {
                return m_parent->FindRowForChild(this);
            }
            else
            {
                return 0;
            }
        }

        GraphCanvasTreeItem* GetParent() const
        {
            return m_parent;
        }

        void RegisterModel(QAbstractItemModel* itemModel, QModelIndex modelIndex)
        {
            AZ_Assert(m_abstractItemModel == nullptr || m_abstractItemModel == itemModel, "GraphCanvasTreeItem being registered to two models at the same time.");
            if (m_abstractItemModel == nullptr)
            {
                m_abstractItemModel = itemModel;
            }

            if (m_abstractItemModel == itemModel)
            {
                if (!m_startModelIndex.isValid() || m_startModelIndex.column() > modelIndex.column())
                {
                    m_startModelIndex = modelIndex;
                }

                if (!m_endModelIndex.isValid() || m_endModelIndex.column() < modelIndex.column())
                {
                    m_endModelIndex = modelIndex;
                }
            }
        }
        
        // Fields that can be customized to manipulate how the tree view behaves.
        virtual int GetColumnCount() const = 0;
        virtual Qt::ItemFlags Flags(const QModelIndex& index) const = 0;
        virtual QVariant Data(const QModelIndex& index, int role) const = 0;
        virtual GraphCanvasMimeEvent* CreateMimeEvent() const { return nullptr; };

        template<class NodeType, class... Params>
        NodeType* CreateChildNode(Params&&... rest)
        {
            NodeType* nodeType = aznew NodeType(AZStd::forward<Params>(rest)...);
            this->AddChild(nodeType);
            return nodeType;
        }

    protected:

        GraphCanvasTreeItem()
            : m_abstractItemModel(nullptr)
            , m_allowSignals(true)
            , m_deleteRemoveChildren(false)
            , m_parent(nullptr)
        {
        }

        int FindRowForChild(const GraphCanvasTreeItem* item) const
        {
            int row = -1;

            for (int i = 0; i < static_cast<int>(m_childItems.size()); ++i)
            {
                if (m_childItems[i] == item)
                {
                    row = i;
                    break;
                }
            }

            AZ_Warning("GraphCanvasTreeItem", row >= 0, "Could not find item in it's parent.");

            return row;
        }

        void RemoveParent(GraphCanvasTreeItem* item)
        {
            AZ_Warning("GraphCanvasTreeItem", m_parent == item, "Trying to remove node from an unknown parent.");
            if (m_parent == item)
            {
                m_parent = nullptr;
            }
        }

        virtual bool LessThan(const GraphCanvasTreeItem* graphItem) const
        {
            return true;
        }

        void AddChild(GraphCanvasTreeItem* item)
        {
            static const Comparator k_insertionComparator = {};

            if (item->m_parent == this)
            {
                return;
            }

            if (item->m_parent != nullptr)
            {
                item->m_parent->RemoveChild(item);
            }

            PreOnChildAdded(item);
            SignalLayoutAboutToBeChanged();

            AZStd::vector< GraphCanvasTreeItem* >::iterator insertPoint = AZStd::lower_bound(m_childItems.begin(), m_childItems.end(), item, k_insertionComparator);
            m_childItems.insert(insertPoint, item);

            item->m_parent = this;
            SignalLayoutChanged();
        }

        void ClearChildren()
        {
            if (m_abstractItemModel)
            {
                m_deleteRemoveChildren = true;
                m_abstractItemModel->removeRows(0, GetNumChildren(), m_startModelIndex);
                m_deleteRemoveChildren = false;
            }
            else
            {
                while (!m_childItems.empty())
                {
                    GraphCanvasTreeItem* item = m_childItems.back();
                    m_childItems.pop_back();

                    delete item;
                }
            }
        }

        void RemoveChild(GraphCanvasTreeItem* item, bool deleteObject = true)
        {
            m_deleteRemoveChildren = deleteObject;

            if (item->m_parent == this)
            {
                // Remove child cannot rely on the comparator being valid.
                // Since the default comparator just returns false.
                for (int i = 0; i < GetNumChildren(); ++i)
                {
                    GraphCanvasTreeItem* currentItem = m_childItems[i];
                    if (currentItem == item)
                    {
                        if (m_abstractItemModel)
                        {
                            m_abstractItemModel->removeRows(i, 1, m_startModelIndex);
                        }
                        else
                        {
                            GraphCanvasTreeItem* treeItem = m_childItems[i];
                            m_childItems.erase(m_childItems.begin() + i);

                            if (deleteObject)
                            {
                                delete treeItem;
                            }
                        }
                        break;
                    }
                }
            }

            m_deleteRemoveChildren = false;
        }

    protected:

        void BlockSignals()
        {
            m_allowSignals = false;
        }

        void UnblockSignals()
        {
            m_allowSignals = true;
        }

        void SignalLayoutAboutToBeChanged()
        {
            if (m_abstractItemModel && m_allowSignals)
            {
                m_abstractItemModel->layoutAboutToBeChanged();
            }
        }

        void SignalLayoutChanged()
        {
            if (m_abstractItemModel && m_allowSignals)
            {
                m_abstractItemModel->layoutChanged();
            }
        }

        void SignalDataChanged()
        {
            if (m_abstractItemModel && m_allowSignals)
            {
                m_abstractItemModel->dataChanged(m_startModelIndex, m_endModelIndex);
            }
        }

        virtual void PreOnChildAdded(GraphCanvasTreeItem* item)
        {
            (void)item;
        }

    private:
        
        QPersistentModelIndex   m_startModelIndex;
        QPersistentModelIndex   m_endModelIndex;
        QAbstractItemModel*     m_abstractItemModel;
        bool m_allowSignals;
        bool m_deleteRemoveChildren;

        GraphCanvasTreeItem* m_parent;
        AZStd::vector< GraphCanvasTreeItem* > m_childItems;
    };
}