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
#include <qpixmap.h>

#include <GraphCanvas/Widgets/NodePalette/TreeItems/IconDecoratedNodePaletteTreeItem.h>
#include <GraphCanvas/Components/StyleBus.h>

namespace GraphCanvas
{
    /////////////////////////////////////
    // IconDecoratedNodePaletteTreeItem
    /////////////////////////////////////
    IconDecoratedNodePaletteTreeItem::IconDecoratedNodePaletteTreeItem(const QString& name, EditorId editorId)
        : NodePaletteTreeItem(name, editorId)
    {
        SetTitlePalette("DefaultNodeTitlePalette");
    }

    QVariant IconDecoratedNodePaletteTreeItem::OnData(const QModelIndex& index, int role) const
    {
        switch (role)
        {
        case Qt::DecorationRole:
            if (m_iconPixmap != nullptr)
            {
                return (*m_iconPixmap);
            }
        default:
            return QVariant();
        }
    }

    void IconDecoratedNodePaletteTreeItem::OnTitlePaletteChanged()
    {
        StyleManagerRequestBus::EventResult(m_iconPixmap, GetEditorId(), &StyleManagerRequests::GetPaletteIcon, "NodePaletteTypeIcon", GetTitlePalette());
    }
}