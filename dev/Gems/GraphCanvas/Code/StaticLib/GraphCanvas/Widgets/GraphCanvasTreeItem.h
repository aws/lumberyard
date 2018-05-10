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

#include <qabstractitemmodel.h>

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/vector.h>
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

        virtual ~GraphCanvasTreeItem();

        void DetachItem();        
        int GetChildCount() const;

        GraphCanvasTreeItem* FindChildByRow(int row) const;
        int FindRowUnderParent() const;

        GraphCanvasTreeItem* GetParent() const;

        void RegisterModel(QAbstractItemModel* itemModel, QModelIndex modelIndex);
        
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

        GraphCanvasTreeItem();

        int FindRowForChild(const GraphCanvasTreeItem* item) const;
        
        void RemoveParent(GraphCanvasTreeItem* item);

        void AddChild(GraphCanvasTreeItem* item);        
        void RemoveChild(GraphCanvasTreeItem* item, bool deleteObject = true);
        void ClearChildren();

        void BlockSignals();
        void UnblockSignals();

        void SignalLayoutAboutToBeChanged();
        void SignalLayoutChanged();
        void SignalDataChanged();
        
        // Overrides for various internal bits of operation
        virtual bool LessThan(const GraphCanvasTreeItem* graphItem) const;
        virtual void PreOnChildAdded(GraphCanvasTreeItem* item);
        
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