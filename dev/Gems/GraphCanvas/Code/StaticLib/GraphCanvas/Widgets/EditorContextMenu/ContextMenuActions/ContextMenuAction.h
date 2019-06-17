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

#include <QAction>
#include <QObject>

#include <AzCore/std/string/string_view.h>
#include <AzCore/std/string/string_view.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/Math/Vector2.h>

#include <GraphCanvas/Editor/EditorTypes.h>

namespace GraphCanvas
{
    typedef AZ::Crc32 ActionGroupId;
    
    class ContextMenuAction
        : public QAction
    {
        Q_OBJECT
    protected:
        ContextMenuAction(AZStd::string_view actionName, QObject* parent);
        
    public:
        // This enum dertermines what actions the scene should take once this action is triggered.
        enum class SceneReaction
        {
            Unknown,
            PostUndo,
            Nothing
        };
        
        virtual ~ContextMenuAction() = default;
        
        virtual ActionGroupId GetActionGroupId() const = 0;
        
        virtual void RefreshAction(const GraphId& graphId, const AZ::EntityId& targetId);

        virtual bool IsInSubMenu() const;
        virtual AZStd::string GetSubMenuPath() const;
        
        // Should trigger the selected action, and return the appropriate reaction for the scene to take.
        virtual SceneReaction TriggerAction(const GraphId& graphId, const AZ::Vector2& scenePos) = 0;
    };
}