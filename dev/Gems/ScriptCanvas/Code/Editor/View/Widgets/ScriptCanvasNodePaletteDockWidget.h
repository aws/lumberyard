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

#include <AzCore/Component/EntityId.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzQtComponents/Components/StyledDockWidget.h>

#include <AzToolsFramework/UI/UICore/QTreeViewStateSaver.hxx>

#include <GraphCanvas/Widgets/NodePalette/NodePaletteDockWidget.h>
#include <GraphCanvas/Editor/AssetEditorBus.h>

namespace ScriptCanvasEditor
{
    namespace Widget
    {
        class NodePaletteDockWidget
            : public GraphCanvas::NodePaletteDockWidget        
        {
        public:
            AZ_CLASS_ALLOCATOR(NodePaletteDockWidget, AZ::SystemAllocator, 0);

            static const char* GetMimeType() { return "scriptcanvas/node-palette-mime-event"; }

            NodePaletteDockWidget(const QString& windowLabel, QWidget* parent, bool inContextMenu = false);
            ~NodePaletteDockWidget() = default;

        protected:

            GraphCanvas::GraphCanvasTreeItem* CreatePaletteRoot() const override;
        };
    }
}
