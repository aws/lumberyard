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
#include <GraphCanvas/Widgets/NodePalette/TreeItems/NodePaletteTreeItem.h>

namespace GraphCanvas
{
    ////////////////////////
    // NodePaletteTreeItem
    ////////////////////////

    NodePaletteTreeItem::NodePaletteTreeItem(const QString& name, EditorId editorId)
        : GraphCanvas::GraphCanvasTreeItem()        
        , m_editorId(editorId)
        , m_name(name)
        , m_highlight(-1, 0)
        , m_ordering(0)
    {
    }

    const QString& NodePaletteTreeItem::GetName() const
    {
        return m_name;
    }

    int NodePaletteTreeItem::GetColumnCount() const
    {
        return 1;
    }

    QVariant NodePaletteTreeItem::Data(const QModelIndex& index, int role) const
    {
        switch (role)
        {
        case Qt::DisplayRole:
            return GetName();
        case Qt::ToolTipRole:
            // If we have a tooltip. Use it
            // Otherwise fall through to use our name.
            if (!m_toolTip.isEmpty())
            {
                return m_toolTip;
            }
        case Qt::EditRole:
            return GetName();
        default:
            break;
        }

        return OnData(index, role);
    }

    Qt::ItemFlags NodePaletteTreeItem::Flags(const QModelIndex& index) const
    {
        Qt::ItemFlags baseFlags = Qt::ItemIsSelectable | Qt::ItemIsEnabled;

        return baseFlags | OnFlags();
    }

    void NodePaletteTreeItem::SetToolTip(const QString& toolTip)
    {
        m_toolTip = toolTip;
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
            for (int i = 0; i < GetChildCount(); ++i)
            {
                NodePaletteTreeItem* childItem = static_cast<NodePaletteTreeItem*>(FindChildByRow(i));

                childItem->SetStyleOverride(styleOverride);
            }
        }

        OnStyleOverrideChange();
    }

    const AZStd::string& NodePaletteTreeItem::GetStyleOverride() const
    {
        return m_styleOverride;
    }

    void NodePaletteTreeItem::SetTitlePalette(const AZStd::string& palette)
    {
        if (m_palette.empty() || m_palette.compare("DefaultNodeTitlePalette") == 0)
        {
            m_palette = palette;

            if (!m_palette.empty())
            {
                for (int i = 0; i < GetChildCount(); ++i)
                {
                    NodePaletteTreeItem* childItem = static_cast<NodePaletteTreeItem*>(FindChildByRow(i));

                    childItem->SetTitlePalette(palette);
                }
            }

            OnTitlePaletteChanged();
        }
    }

    const AZStd::string& NodePaletteTreeItem::GetTitlePalette() const
    {
        return m_palette;
    }

    void NodePaletteTreeItem::SetHighlight(const AZStd::pair<int, int>& highlight)
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

    void NodePaletteTreeItem::PreOnChildAdded(GraphCanvasTreeItem* item)
    {
        if (!m_styleOverride.empty())
        {
            static_cast<NodePaletteTreeItem*>(item)->SetStyleOverride(m_styleOverride);
        }

        if (!m_palette.empty())
        {
            static_cast<NodePaletteTreeItem*>(item)->SetTitlePalette(m_palette);
        }
    }

    void NodePaletteTreeItem::SetName(const QString& name)
    {
        m_name = name;
        SignalDataChanged();
    }

    const EditorId& NodePaletteTreeItem::GetEditorId() const
    {
        return m_editorId;
    }

    bool NodePaletteTreeItem::LessThan(const GraphCanvasTreeItem* graphItem) const
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

    QVariant NodePaletteTreeItem::OnData(const QModelIndex& index, int role) const
    {
        return QVariant();
    }

    Qt::ItemFlags NodePaletteTreeItem::OnFlags() const
    {
        return Qt::ItemFlags();
    }

    void NodePaletteTreeItem::OnStyleOverrideChange()
    {
    }

    void NodePaletteTreeItem::OnTitlePaletteChanged()
    {
    }
}