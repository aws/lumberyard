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

#include <QVariant>
#include <QIcon>

#include <AzCore/Memory/SystemAllocator.h>

#include <GraphCanvas/Widgets/GraphCanvasTreeItem.h>

namespace ScriptCanvasEditor
{
    class NodePaletteTreeItem
        : public GraphCanvas::GraphCanvasTreeItem
    {
    public:
        AZ_CLASS_ALLOCATOR(NodePaletteTreeItem, AZ::SystemAllocator, 0);
        AZ_RTTI(NodePaletteTreeItem, "{D1BAAF63-F823-4D2A-8F55-01AC2A659FF5}", GraphCanvas::GraphCanvasTreeItem);

        NodePaletteTreeItem(const QString& name);
        ~NodePaletteTreeItem() = default;

        void SetToolTip(const QString& toolTip);
        const QString& GetName() const;

        void SetHighlight(const AZStd::pair<int, int>& highlight);
        bool HasHighlight() const;
        const AZStd::pair<int, int>& GetHighlight() const;
        void ClearHighlight();

        int GetColumnCount() const override;
        QVariant Data(const QModelIndex& index, int role) const override final;
        Qt::ItemFlags Flags(const QModelIndex& index) const override final;

        void SetItemOrdering(int ordering);

        void SetStyleOverride(const AZStd::string& styleOverride);
        const AZStd::string& GetStyleOverride() const;

    protected:

        void PreOnChildAdded(GraphCanvasTreeItem* item) override;

        void SetName(const QString& name);

        virtual bool LessThan(const GraphCanvasTreeItem* graphItem) const
        {
            const NodePaletteTreeItem* otherItem = static_cast<const NodePaletteTreeItem*>(graphItem);
            if (m_ordering == otherItem->m_ordering)
            {
                return m_name < static_cast<const NodePaletteTreeItem*>(graphItem)->m_name;
            }
            else
            {
                return m_ordering < otherItem->m_ordering;
            }
        }

        virtual QVariant OnData(const QModelIndex& index, int role) const;
        virtual Qt::ItemFlags OnFlags() const;

    private:

        AZStd::string m_styleOverride;
        QString m_name;
        QString m_toolTip;

        AZStd::pair<int, int> m_highlight;

        int m_ordering;
    };
        
    class DraggableNodePaletteTreeItem
        : public NodePaletteTreeItem
    {
    public:
        AZ_CLASS_ALLOCATOR(DraggableNodePaletteTreeItem, AZ::SystemAllocator, 0);
        AZ_RTTI(DraggableNodePaletteTreeItem, "{40D29F3E-17F5-42B5-B771-FFAD7DB3CB96}", NodePaletteTreeItem);

        DraggableNodePaletteTreeItem(const QString& name, const QString& iconPath);
        ~DraggableNodePaletteTreeItem() = default;
            
    protected:

        Qt::ItemFlags OnFlags() const override;
        QVariant OnData(const QModelIndex& index, int role) const override;

    private:
        QIcon m_icon;
    };
}
