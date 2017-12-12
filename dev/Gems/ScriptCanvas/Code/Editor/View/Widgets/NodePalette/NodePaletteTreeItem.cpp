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

#include <QMimeData>

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

#include "NodePaletteTreeItem.h"

#include <Core/Attributes.h>
#include <Libraries/Libraries.h>

namespace ScriptCanvasEditor
{
    ////////////////////////
    // NodePaletteTreeItem
    ////////////////////////

    NodePaletteTreeItem::NodePaletteTreeItem(const QString& name)
        : GraphCanvas::GraphCanvasTreeItem()
        , m_name(name)
        , m_highlight(-1,0)
        , m_ordering(0)
    {
    }
        
    void NodePaletteTreeItem::SetToolTip(const QString& toolTip)
    {
        m_toolTip = toolTip;
    }

    const QString& NodePaletteTreeItem::GetName() const
    {
        return m_name;
    }

    void NodePaletteTreeItem::SetHighlight(const AZStd::pair<int,int>& highlight)
    {
        m_highlight = highlight;
    }

    bool NodePaletteTreeItem::HasHighlight() const
    {
        return m_highlight.first >= 0 && m_highlight.second > 0;
    }

    const AZStd::pair<int, int>& NodePaletteTreeItem::GetHighlight() const
    {
        return m_highlight;
    }

    void NodePaletteTreeItem::ClearHighlight()
    {
        m_highlight.first = -1;
        m_highlight.second = 0;
    }

    int NodePaletteTreeItem::GetColumnCount() const
    {
        return 1;
    }

    QVariant NodePaletteTreeItem::Data(const QModelIndex& index, int row) const
    {
        switch (row)
        {
        case Qt::DisplayRole:
            return GetName();
        case Qt::ToolTipRole:
            if (!m_toolTip.isEmpty())
            {
                return m_toolTip;
            }
        }

        return OnData(index, row);
    }

    Qt::ItemFlags NodePaletteTreeItem::Flags(const QModelIndex& modelIndex) const
    {
        Qt::ItemFlags baseFlags = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
            
        return baseFlags | OnFlags();
    }

    void NodePaletteTreeItem::SetItemOrdering(int ordering)
    {
        m_ordering = ordering;
        SignalLayoutChanged();
    }

    void NodePaletteTreeItem::SetStyleOverride(const AZStd::string& styleOverride)
    {        
        m_styleOverride = styleOverride;

        if (!m_styleOverride.empty())
        {
            for (int i = 0; i < GetNumChildren(); ++i)
            {
                NodePaletteTreeItem* childItem = static_cast<NodePaletteTreeItem*>(ChildForRow(i));

                childItem->SetStyleOverride(m_styleOverride);
            }
        }
    }

    const AZStd::string& NodePaletteTreeItem::GetStyleOverride() const
    {
        return m_styleOverride;
    }

    void NodePaletteTreeItem::PreOnChildAdded(GraphCanvasTreeItem* item)
    {
        if (!m_styleOverride.empty())
        {
            static_cast<NodePaletteTreeItem*>(item)->SetStyleOverride(m_styleOverride);
        }
    }

    void NodePaletteTreeItem::SetName(const QString& name)
    {
        m_name = name;
        SignalDataChanged();
    }

    QVariant NodePaletteTreeItem::OnData(const QModelIndex&, int) const
    {
        return QVariant();
    }

    Qt::ItemFlags NodePaletteTreeItem::OnFlags() const
    {
        return 0;
    }
        
    /////////////////////////////////
    // DraggableNodePaletteTreeItem
    /////////////////////////////////
        
    DraggableNodePaletteTreeItem::DraggableNodePaletteTreeItem(const QString& name, const QString& iconPath)
        : NodePaletteTreeItem(name)
        , m_icon(iconPath)
    {
    }
        
    Qt::ItemFlags DraggableNodePaletteTreeItem::OnFlags() const
    {
        return Qt::ItemIsDragEnabled;
    }

    QVariant DraggableNodePaletteTreeItem::OnData(const QModelIndex& index, int role) const
    {
        if (role == Qt::DecorationRole)
        {
            // TODO: Unhide icons. This is TEMPORARY, per jegachen's request. See: LY-57166
            //return m_icon;
        }

        return QVariant();
    }
}
