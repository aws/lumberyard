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

#include <AzCore/RTTI/RTTI.h>

#include <GraphCanvas/Widgets/NodePalette/TreeItems/NodePaletteTreeItem.h>

namespace GraphCanvas
{
    class IconDecoratedNodePaletteTreeItem
        : public NodePaletteTreeItem
    {
    public:
        AZ_CLASS_ALLOCATOR(IconDecoratedNodePaletteTreeItem, AZ::SystemAllocator, 0);
        AZ_RTTI(IconDecoratedNodePaletteTreeItem, "{674FE7BB-C15C-4532-B580-336C7C6173A3}", NodePaletteTreeItem);

        IconDecoratedNodePaletteTreeItem(const QString& name, EditorId editorId);
        ~IconDecoratedNodePaletteTreeItem() override = default;
            
    protected:

        QVariant OnData(const QModelIndex& index, int role) const override;

        void OnTitlePaletteChanged() override;

    private:        
        const QPixmap* m_iconPixmap;
    };
}