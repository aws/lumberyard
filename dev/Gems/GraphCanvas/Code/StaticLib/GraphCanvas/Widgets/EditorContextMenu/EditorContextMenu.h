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

#include <QMenu>

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/unordered_set.h>

#include <GraphCanvas/Widgets/EditorContextMenu/contextMenuActions/ContextMenuAction.h>

namespace GraphCanvas
{
    class EditorContextMenu
        : public QMenu
    {
        Q_OBJECT
    public:    
        AZ_CLASS_ALLOCATOR(EditorContextMenu, AZ::SystemAllocator, 0);
        
        EditorContextMenu(QWidget* parent = nullptr);
        virtual ~EditorContextMenu() = default;
        
        void AddActionGroup(const ActionGroupId& actionGroup);
        void AddMenuAction(QAction* contextMenuAction);
        
        void RefreshActions(const GraphId& graphId, const AZ::EntityId& targetMemberId);
        
        void showEvent(QShowEvent* showEvent) override;
        
    protected:
        virtual void OnRefreshActions(const GraphId& graphId, const AZ::EntityId& targetMemberId);
        
    private:
    
        void ConstructMenu();
    
        bool m_finalized;

        AZStd::vector< ActionGroupId >          m_actionGroupOrdering;
        AZStd::unordered_set< ActionGroupId >   m_actionGroups;
        
        AZStd::vector< QAction* > m_unprocessedActions;
        AZStd::unordered_map< AZStd::string, QMenu* > m_subMenuMap;
    };
}