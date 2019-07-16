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
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenus/SceneContextMenu.h>

#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/SceneMenuActions/SceneContextMenuActions.h>

#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/NodeGroupMenuActions/NodeGroupContextMenuActions.h>

namespace GraphCanvas
{
    /////////////////////
    // SceneContextMenu
    /////////////////////
    
    SceneContextMenu::SceneContextMenu(QWidget* parent)
        : EditorContextMenu(parent)
    {
        m_editorActionsGroup.PopulateMenu(this);
        m_graphCanvasConstructGroups.PopulateMenu(this);

        AddActionGroup(NodeGroupContextMenuAction::GetNodeGroupContextMenuActionGroupId());
        AddMenuAction(aznew CreateNewNodeGroupMenuAction(this));

        m_alignmentActionsGroups.PopulateMenu(this);
    }
}